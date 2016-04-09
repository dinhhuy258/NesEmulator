#include <GL/glut.h>
#include "PPU.h"
#include "Cartridge.h"
#include "Platforms.h"
#include "Mapper.h"
#include "MemoryCPU.h"
#include "MemoryPPU.h"
#include "CPU.h"
#include "Palette.h"

// NES components
Cartridge *cartridge;
Mapper *mapper;
Memory *memoryPPU;
PPU *ppu;
Memory *memoryCPU;
CPU *cpu;
// Delta time
uint32_t oldTime;
// Window size
int modifier = 2;
int displayWidth = SCREEN_WIDTH * modifier;
int displayHeight = SCREEN_HEIGHT * modifier;
uint8_t screenData[SCREEN_HEIGHT][SCREEN_WIDTH][3]; 

double GetDeltaTime();
void StepSeconds(double deltaTime);
uint8_t Step();
void SetupTexture();
void Display();
void ReshapeWindow(GLsizei w, GLsizei h);

int main(int argc, char **argv)
{
    // Init NES components
    cartridge = new Cartridge();
    if (cartridge->LoadNESFile(NES_FILE) == false)
    {
        LOGI("Invalid NES file");
        return 0;
    }
    Mapper *mapper = Mapper::GetMapper(cartridge);
    if (mapper == NULL)
    {
        LOGI("We haven't supported this mapper");
        return 0;
    }
    memoryPPU = new MemoryPPU();
    memoryPPU->SetMapper(mapper);
    ppu = new PPU(memoryPPU);

    memoryCPU = new MemoryCPU();
    memoryCPU->SetMapper(mapper);
    memoryCPU->SetPPU(ppu);
    cpu = new CPU(memoryCPU);
    ppu->SetCPU(cpu);
    // Init time
    oldTime = 0;
    // Init GLUT and create window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(displayWidth, displayHeight);
    glutInitWindowPosition(100,100);
    glutCreateWindow("NesEmulator by Huy Duong");
    // Register call back function
    glutDisplayFunc(Display);
    glutIdleFunc(Display);
    glutReshapeFunc(ReshapeWindow);
    // Setup texture
    SetupTexture();
    // Enter GLUT event processing loop
    glutMainLoop();
    // Deallocate
    SAFE_DEL(cartridge);
    SAFE_DEL(mapper);
    SAFE_DEL(memoryPPU);
    SAFE_DEL(ppu);
    SAFE_DEL(memoryCPU);
    SAFE_DEL(cpu);
    return 1;
}

double GetDeltaTime()
{
    uint32_t newTime = glutGet(GLUT_ELAPSED_TIME);
    double result = (newTime - oldTime) / 1000.0;
    oldTime = newTime;
    return result;
}

void SetupTexture()
{
    // Clear screen
    for(int y = 0; y < SCREEN_HEIGHT; ++y)  
    {
        for(int x = 0; x < SCREEN_WIDTH; ++x)
        {
            screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;
        }
    }
 
    // Create a texture 
    glTexImage2D(GL_TEXTURE_2D, 0, 3, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);
    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
    // Enable textures
    glEnable(GL_TEXTURE_2D);
}


void ReshapeWindow(GLsizei w, GLsizei h)
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
    // Resize quad
    displayWidth = w;
    displayHeight = h;
}

void UpdateTexture()
{   
    // Update pixels
    for(int y = 0; y < SCREEN_HEIGHT; ++y)  
    {
        for(int x = 0; x < SCREEN_WIDTH; ++x)
        {
            uint32_t color = palette[ppu->paletteIndex[y][x]];
            screenData[y][x][0] = uint8_t(color >> 16);
            screenData[y][x][1] = uint8_t(color >> 8);
            screenData[y][x][2] = uint8_t(color);
        }
    }
    // Update Texture
    glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);
    glBegin(GL_QUADS);
        glTexCoord2d(0.0, 0.0);     glVertex2d(0.0,           0.0);
        glTexCoord2d(1.0, 0.0);     glVertex2d(displayWidth,  0.0);
        glTexCoord2d(1.0, 1.0);     glVertex2d(displayWidth,  displayHeight);
        glTexCoord2d(0.0, 1.0);     glVertex2d(0.0,           displayHeight);
    glEnd();
}

void Display()
{
    double deltaTime = GetDeltaTime();
    StepSeconds(deltaTime);
    // Clear framebuffer
    glClear(GL_COLOR_BUFFER_BIT);
    UpdateTexture(); 
    glutSwapBuffers();
}

void StepSeconds(double deltaTime)
{
    int32_t cycles =  uint32_t(CPU_FREQUENCY * deltaTime);
    while(cycles > 0)
    {
        cycles -= Step();
    }
}

uint8_t Step()
{
    uint8_t cpuCycles = cpu->Step();
    uint8_t ppuCycles = cpuCycles * 3;
    for (uint8_t i = 0; i < ppuCycles; ++i)
    {
        ppu->Step();  
    }
    return cpuCycles;
}