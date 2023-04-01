#ifndef JOYSTICKINTERFACE_HPP_
#define JOYSTICKINTERFACE_HPP_

#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <linux/joystick.h>
#include <iostream>
#include <thread>

#define AXIS_MAX 32767
#define LEFT_JOYSTICK_Y 0
#define LEFT_JOYSTICK_X 1
#define RIGHT_JOYSTICK_Y 2
#define LEFT_TRIGGER 3
#define RIGHT_TRIGGER 4
#define RIGHT_JOYSTICK_X 5
#define DPAD_Y 6
#define DPAD_X 7

struct joystickButtons
{
    joystickButtons() : leftJoy(2, 0.0), rightJoy(2, 0.0), 
                        trigger(2, 0.0), buttons(15, false) {}
    int dpad_y;
    int dpad_x;
    std::vector <double> leftJoy;
    std::vector <double> rightJoy;
    std::vector <double> trigger;
    std::vector <bool> buttons;
};

enum joyaxis
{
    x = 0,
    y = 1
};

enum trigger
{
    left = 0,
    right = 1
};

class joystickInterface
{
public:
    joystickInterface()
    {
        joystick = open(device_name.c_str(), O_RDONLY);
        if (joystick == -1)
            perror("Could not open joystick");
    }

    joystickInterface(std::string device_, bool debug_)
    {
        debug = debug_;
        device_name = device_;
        joystick = open(device_.c_str(), O_RDONLY);
        if (joystick == -1)
            perror("Could not open joystick");
    }

    bool makeJoystickThread()
    {
        if (joystick != -1)
        {
            std::cout << "Creating the thread for Joystick" << std::endl;
            joy_thread = std::thread(&joystickInterface::updateJoystickStruct, this);
            threadRunning = true;
            return threadRunning;
        }
        return false;
    }

    joystickButtons getJoystickStruct()
    {
        if (!threadRunning) retryConnection();
        return joy_;
    }

    ~joystickInterface()
    {
        close(joystick);
        joy_thread.join();
    }

private:
    int joystick;
    int failCounter;
    struct js_event event;
    joystickButtons joy_;
    std::thread joy_thread;
    bool debug = false;
    bool threadRunning = false;
    int step_count = 0;
    std::string device_name = "/dev/input/js0";

    void retryConnection()
    {
        step_count++;
        if (step_count == 100)
        {
            joystick = open(device_name.c_str(), O_RDONLY);
            if (joystick == -1) 
                perror("Could not open joystick");
            else
            {
                std::cout << "Recreating joystick thread" << std::endl;
                step_count = 0;
                makeJoystickThread();
            }
        }
    }

    /**
     * Read the joystick data and update the struct.
     */
    void updateJoystickStruct()
    {
        while (failCounter <= 10)
        {
            if (read_event(joystick, &event) != 0) failCounter++;
            switch (event.type)
            {
            case JS_EVENT_BUTTON:
                joy_.buttons[event.number] = (event.value == 1);
                if (debug) 
                    std::cout << "Button " << static_cast<unsigned>(event.number) << " is: " << joy_.buttons[event.number] << std::endl;
                break;
            case JS_EVENT_AXIS:
                set_axis_state(&event);
                break;
            default:
                break;
            }
        }
        perror("Could not read joystick");
        setJoystickToZero();
        bool threadRunning = false;
    }

    void setJoystickToZero()
    {
        joy_ = joystickButtons();
    }

    /**
     * Reads a joystick event from the joystick device.
     *
     * Returns 0 on success. Otherwise -1 is returned.
     */
    int read_event(int fd, struct js_event *event)
    {
        ssize_t bytes;

        bytes = read(fd, event, sizeof(*event));

        if (bytes == sizeof(*event))
            return 0;

        /* Error, could not read full event. */
        return -1;
    }

    /**
     * Returns the number of axes on the controller or 0 if an error occurs.
     */
    size_t get_axis_count(int fd)
    {
        __u8 axes;

        if (ioctl(fd, JSIOCGAXES, &axes) == -1)
            return 0;

        return axes;
    }

    /**
     * Returns the number of buttons on the controller or 0 if an error occurs.
     */
    size_t get_button_count(int fd)
    {
        __u8 buttons;
        if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
            return 0;

        return buttons;
    }

    /**
     * Keeps track of the current axis state.
     */
    void set_axis_state(struct js_event *event)
    {
        switch (event->number)
        {
        case LEFT_JOYSTICK_Y:
            joy_.leftJoy[y] = static_cast<float>(event->value) / AXIS_MAX;
            if (debug) std::cout << "Left joystick Y move " << joy_.leftJoy[y] << std::endl;
            break;
        case LEFT_JOYSTICK_X:
            joy_.leftJoy[x] = - static_cast<float>(event->value) / AXIS_MAX;
            if (debug) std::cout << "Left joystick X move " << joy_.leftJoy[x] << std::endl;
            break;
        case RIGHT_JOYSTICK_Y:
            joy_.rightJoy[y] = static_cast<float>(event->value) / AXIS_MAX;
            if (debug) std::cout << "Right joystick Y move " << joy_.rightJoy[y] << std::endl;
            break;
        case LEFT_TRIGGER:
            joy_.trigger[left] = (static_cast<float>(event->value) + AXIS_MAX) / (2 * AXIS_MAX);
            if (debug) std::cout << "Left trigger move " << joy_.trigger[left] << std::endl;
            break;
        case RIGHT_TRIGGER:
            joy_.trigger[right] = (static_cast<float>(event->value) + AXIS_MAX) / (2 * AXIS_MAX);
            if (debug) std::cout << "Right trigger move " << joy_.trigger[right] << std::endl;
            break;
        case RIGHT_JOYSTICK_X:
            joy_.rightJoy[x] = - static_cast<float>(event->value) / AXIS_MAX;
            if (debug) std::cout << "Right joystick X move " << joy_.rightJoy[x] << std::endl;
            break;
        case DPAD_Y:
            joy_.dpad_y = event->value / AXIS_MAX;
            if (debug) std::cout << "Dpad lateral move " << joy_.dpad_y << std::endl;
            break;
        case DPAD_X:
            joy_.dpad_x = - event->value / AXIS_MAX;
            if (debug) std::cout << "Dpad longitidunal move " << joy_.dpad_x << std::endl;
            break;
        default:
            break;
        }
    }
};

#endif  // JOYSTICKINTERFACE_HPP_