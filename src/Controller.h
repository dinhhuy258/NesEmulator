#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <stdint.h>

enum ButtonType
{
    ButtonA = 0,
    ButtonB,
    ButtonSelect,
    ButtonStart,
    ButtonUp,
    ButtonDown,
    ButtonLeft,
    ButtonRight
};
#define MAX_BUTTON_TYPE ((ButtonType) (ButtonRight + 1))

class Controller
{
    public:
        Controller();
        void SetButton(ButtonType buttonType, bool isPressed);
        void Write(uint8_t value);
        uint8_t Read();

    private:
        bool button[MAX_BUTTON_TYPE];
        /*
         * 7  bit  0
         * ---- ----
         * xxxx xxxS
         *         |
         *         +- Controller shift register strobe
         */
        uint8_t controllerRegister;
        uint8_t index;
};

#endif //_CONTROLLER_H_