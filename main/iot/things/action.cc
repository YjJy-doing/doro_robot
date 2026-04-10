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
    Action() : Thing("Action", "机器人动作。机器人可以做以下动作：\n"
                                         "Walk: 向前移动四步\n"
                                         "Walk back: 向后移动四步\n"
                                         "Turn left: 向左转90度\n"
                                         "Turn right: 向右转90度\n"
                                         "stand at attention: 立正\n"
                                         "sitdown: 坐下\n"
                                         "sleep: 睡觉\n"
                                         "show talent: 展示才艺\n"
                                         "hello: 打招呼\n"
                                         "bye-bye: 再见\n"
                                         "stop: 立即停止当前动作") {
        methods_.AddMethod("Walk", "前进", ParameterList(), [this](const ParameterList& parameters) {
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

        methods_.AddMethod("turn left", "向左转", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateTurnLeft90);
        });

        methods_.AddMethod("turn right", "向右转", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateTurnRight90);
        });

        methods_.AddMethod("show talent", "展示才艺", ParameterList(), [this](const ParameterList& parameters) {
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

        methods_.AddMethod("stop", "停下", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.SetActionState(kActionStateStop);
        });
    }
};

} // namespace iot

DECLARE_THING(Action);