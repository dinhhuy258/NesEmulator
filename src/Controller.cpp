#include "Controller.h"

Controller::Controller()
{
    for (uint8_t i = 0; i < MAX_BUTTON_TYPE; ++i)
    {
        button[i] = false;
    }
    index = 0;
}

void Controller::SetButton(ButtonType buttonType, bool isPressed)
{
    button[buttonType] = isPressed;
}

void Controller::Write(uint8_t value)
{

    /*
     * 7  bit  0
     * ---- ----
     * xxxx xxxS
     *         |
     *         +- Controller shift register strobe
     *
     * Writing 1 to this bit will record the state of each button on the controller in the following
     * order A, B, Select, Start, Up, Down, Left, Right 
     * Writing 0 afterwards will allow the buttons to be read back, one at a time
     */
    controllerRegister = value;
    if  ((controllerRegister & 0x01) == 0x01)
    {
        /*
         * While S (strobe) is high, the shift registers in the controllers are continuously
         * reloaded from the button states, and reading $4016/$4017 will keep returning the 
         * current state of the first button (A)
         * Once S goes low, this reloading will stop. Hence a 1/0 write sequence is required 
         * to get the button states, after which the buttons can be read back one at a time
         */
        index = 0;
    }
}

uint8_t Controller::Read()
{
    // Return the current state of button[index]. 1 if pressed, 0 if not pressed.
    uint8_t result = 0;
    if ((index < MAX_BUTTON_TYPE) && (button[index]))
    {
        result = true;
    }
    ++index;
    if  ((controllerRegister & 0x01) == 0x01)
    {
        /*
         * While S (strobe) is high, the shift registers in the controllers are continuously
         * reloaded from the button states, and reading $4016/$4017 will keep returning the 
         * current state of the first button (A)
         */
        index = 0;
    }
    return result;
}