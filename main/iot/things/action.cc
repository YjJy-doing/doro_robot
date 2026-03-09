#include "driver/gpio.h"
#include <esp_log.h>

#include "pet_dog.h"
#include "application.h"
#include "board.h"
#include "iot/thing.h"

#define TAG "Action"


namespace iot {

class Action : public Thing {
private:

    
public:
    Action() : Thing("Action", "当前 AI 机器人的行为（站立，坐下，睡觉，持续左转，持续右转，向左转90度，向右转90度，前进，前进四，后退，展示才艺）") {
        // 定义设备可以被远程执行的指令
        methods_.AddMethod("Walk", "前进", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateWalk);
        });

        methods_.AddMethod("Walk four steps", "前进四", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateWalkFour);
        });

        methods_.AddMethod("Walk back", "后退", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateWalkBack);
        });
        methods_.AddMethod("stand at attention", "立正", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateStand);
        });
        methods_.AddMethod("sitdown", "坐下", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateSitdown);
        });
        methods_.AddMethod("sleep", "睡觉", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateSleep);
        });

        methods_.AddMethod("turn left", "左转（持续旋转，直到停下来）", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateTurnLeft);
        });

        methods_.AddMethod("turn right", "右转（持续旋转，直到停下来）", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateTurnRight);
        });

        methods_.AddMethod("turn left 90", "向左转（左转90度后停下）", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateTurnLeft90);
        });

        methods_.AddMethod("turn right 90", "向右转（右转90度后停下）", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateTurnRight90);
        });

        methods_.AddMethod("show talent", "展示才艺（前后摇摆）", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateSway);
        });

        methods_.AddMethod("bye-bye", "再见", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateWave);
        });

        methods_.AddMethod("hello", "你好", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateWave);
        });

        methods_.AddMethod("stop", "停下来", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateStop);
        });
    }
};

} // namespace iot

DECLARE_THING(Action);