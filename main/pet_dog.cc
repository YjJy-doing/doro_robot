#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pet_dog.h"
#include "application.h"

PetDog::PetDog()
{
    ledc_timer_ = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY, 
        .clk_cfg          = LEDC_AUTO_CLK 
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_));
}

PetDog::~PetDog()
{
    ledc_stop(LEDC_MODE, CHANNEL_0, 0);
    ledc_stop(LEDC_MODE, CHANNEL_1, 0);
    ledc_stop(LEDC_MODE, CHANNEL_2, 0);
    ledc_stop(LEDC_MODE, CHANNEL_3, 0);
}

void PetDog::InitializeDog(gpio_num_t LEDC_OUTPUT_IO_1, gpio_num_t LEDC_OUTPUT_IO_2, gpio_num_t LEDC_OUTPUT_IO_3, gpio_num_t LEDC_OUTPUT_IO_4)
{
    action_task_event_ = xEventGroupCreate();
    //配置通道0
    ledc_channel_config_t ledc_channel_0 = {
        .gpio_num       = LEDC_OUTPUT_IO_1,
        .speed_mode     = LEDC_MODE,
        .channel        = CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_0));
    //配置通道1
    ledc_channel_config_t ledc_channel_1 = {
        .gpio_num       = LEDC_OUTPUT_IO_2,
        .speed_mode     = LEDC_MODE,
        .channel        = CHANNEL_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_1));
    //配置通道2
    ledc_channel_config_t ledc_channel_2 = {
        .gpio_num       = LEDC_OUTPUT_IO_3, 
        .speed_mode     = LEDC_MODE,
        .channel        = CHANNEL_2,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_2));
    //配置通道3
    ledc_channel_config_t ledc_channel_3 = {
        .gpio_num       = LEDC_OUTPUT_IO_4,
        .speed_mode     = LEDC_MODE,
        .channel        = CHANNEL_3,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_3)); 

    
    lf_.index = LEG1;
    rf_.index = LEG2;
    lb_.index = LEG3;
    rb_.index = LEG4;
    
    lf_.speed = 10;
    rf_.speed = 10;
    lb_.speed = 10;
    rb_.speed = 10;

    xTaskCreate([](void* arg)
    {
        auto this_ = (PetDog*)arg;
        this_->ActionTask();
        vTaskDelete(NULL);
    },"actionTask",2048,this,1,nullptr);

}

void PetDog::OnActionTask(std::function<void()> callback)
{
    action_task_ = callback;
}

void PetDog::ActionTask()
{
    // Single dispatcher loop: Application updates action_state_, this task polls
    // and triggers corresponding movement routines.
    while (1)
    {
        if(action_task_ != nullptr)
        {
            action_task_();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }else
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

void PetDog::ActionIdleTask()
{
    auto& app = Application::GetInstance();
    int rand_time = 0;
    int rand_action = 0;
    
    while (1) {
        DeviceState dev_state = app.GetDeviceState();
        if (dev_state == kDeviceStateIdle) {
            rand_time = (rand() % 121) + 60;        //60-180
            // rand_time = (rand() % 31) + 2;        //test:2-32
            // printf("rand_time:%d\r\n",rand_time);
            rand_time *= 60; 
            rand_action = rand() % 3;
        }
        while(rand_time > 0)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            dev_state = app.GetDeviceState();
            if (dev_state != kDeviceStateIdle)
            {
                rand_time = 0;
                break;
            }else
            {
                // printf("the rest of rand_time:%d\r\n",rand_time);
                rand_time --;
                if (rand_time == 0)
                {
                    idle_activate(rand_action);
                }
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void PetDog::idle_activate(int rand_action)
{
    auto display = Board::GetInstance().GetDisplay();
    display->start_emtion();
    if(rand_action == 0)
    {
        stretch();
    }else if (rand_action == 1)
    {
        stretch2();
    }else if (rand_action == 2)
    {
        scratching();
    }else
    {
        swayFrontBack();
    }
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    petsleep();
    display->idle_emtion();
}

bool PetDog::should_stop() const
{
    // STOP bit is set by high-level state transitions (sleep/stand/stop etc.).
    return (xEventGroupGetBits(action_task_event_) & STOP_TASK_EVENT) != 0;
}

bool PetDog::WalkFrontCycle()
{
    if (should_stop()) {
        return false;
    }
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF-WALK_RANGE, STAND_ANGLE_LB-WALK_RANGE, STAND_ANGLE_RB);
    set_angle(STAND_ANGLE_LF+WALK_RANGE, STAND_ANGLE_RF-WALK_RANGE, STAND_ANGLE_LB-WALK_RANGE, STAND_ANGLE_RB+WALK_RANGE);
    set_angle(STAND_ANGLE_LF+WALK_RANGE, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB+WALK_RANGE);
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB);
    set_angle(STAND_ANGLE_LF-WALK_RANGE, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB-WALK_RANGE);
    set_angle(STAND_ANGLE_LF-WALK_RANGE, STAND_ANGLE_RF+WALK_RANGE, STAND_ANGLE_LB+WALK_RANGE, STAND_ANGLE_RB-WALK_RANGE);
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF+WALK_RANGE, STAND_ANGLE_LB+WALK_RANGE, STAND_ANGLE_RB);
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB);
    return !should_stop();
}

bool PetDog::WalkBackCycle()
{
    if (should_stop()) {
        return false;
    }
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB);
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF+WALK_RANGE, STAND_ANGLE_LB+WALK_RANGE, STAND_ANGLE_RB);
    set_angle(STAND_ANGLE_LF-WALK_RANGE, STAND_ANGLE_RF+WALK_RANGE, STAND_ANGLE_LB+WALK_RANGE, STAND_ANGLE_RB-WALK_RANGE);
    set_angle(STAND_ANGLE_LF-WALK_RANGE, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB-WALK_RANGE);
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB);
    set_angle(STAND_ANGLE_LF+WALK_RANGE, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB+WALK_RANGE);
    set_angle(STAND_ANGLE_LF+WALK_RANGE, STAND_ANGLE_RF-WALK_RANGE, STAND_ANGLE_LB-WALK_RANGE, STAND_ANGLE_RB+WALK_RANGE);
    set_angle(STAND_ANGLE_LF, STAND_ANGLE_RF-WALK_RANGE, STAND_ANGLE_LB-WALK_RANGE, STAND_ANGLE_RB);
    return !should_stop();
}

bool PetDog::TurnLeftCycle()
{
    if (should_stop()) {
        return false;
    }
    set_right_front_angle(STAND_ANGLE_RF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);

    set_right_front_angle(STAND_ANGLE_RF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);

    set_right_front_angle(STAND_ANGLE_RF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);

    set_right_front_angle(STAND_ANGLE_RF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    return !should_stop();
}

bool PetDog::TurnRightCycle()
{
    if (should_stop()) {
        return false;
    }
    set_right_front_angle(STAND_ANGLE_RF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);

    set_right_front_angle(STAND_ANGLE_RF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);

    set_right_front_angle(STAND_ANGLE_RF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB-TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF+TURN_RANGE);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);

    set_right_front_angle(STAND_ANGLE_RF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_right_back_angle(STAND_ANGLE_RB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_front_angle(STAND_ANGLE_LF);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    set_left_back_angle(STAND_ANGLE_LB);
    vTaskDelay(SPEED / portTICK_PERIOD_MS);
    return !should_stop();
}

void PetDog::stand()
{
    to_any_angle_task(STAND_ANGLE_LF,STAND_ANGLE_RF,STAND_ANGLE_LB,STAND_ANGLE_RB);
    xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
}

void PetDog::sitdown()
{
    to_any_angle_task(SITDOWN_ANGLE_LF,SITDOWN_ANGLE_RF,SITDOWN_ANGLE_LB,SITDOWN_ANGLE_RB);
    xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
}

void PetDog::petsleep()
{
    to_any_angle_task(SLEEP_ANGLE_LF,SLEEP_ANGLE_RF,SLEEP_ANGLE_LB,SLEEP_ANGLE_RB);
    xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ledc_stop(LEDC_MODE, CHANNEL_0, 0);
    ledc_stop(LEDC_MODE, CHANNEL_1, 0);
    ledc_stop(LEDC_MODE, CHANNEL_2, 0);
    ledc_stop(LEDC_MODE, CHANNEL_3, 0);
}

void PetDog::stretch()
{
    lf_.speed = 20;
    rf_.speed = 20;
    lb_.speed = 20;
    rb_.speed = 20;
    to_any_angle_task(STAND_ANGLE_LF,STAND_ANGLE_RF,STAND_ANGLE_LB,STAND_ANGLE_RB);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    to_any_angle_task(10,10,45,45);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    to_any_angle_task(135,135,170,170);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    to_any_angle_task(STAND_ANGLE_LF,STAND_ANGLE_RF,STAND_ANGLE_LB,STAND_ANGLE_RB);

    lf_.speed = 10;
    rf_.speed = 10;
    lb_.speed = 10;
    rb_.speed = 10;

    xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
}

void PetDog::stretch2()
{
    stop();
    lf_.speed = 20;
    rf_.speed = 20;
    lb_.speed = 20;
    rb_.speed = 20;
    to_any_angle_task(STAND_ANGLE_LF,STAND_ANGLE_RF,STAND_ANGLE_LB,STAND_ANGLE_RB);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    to_any_angle_task(0,0,180,180);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    to_any_angle_task(STAND_ANGLE_LF,STAND_ANGLE_RF,STAND_ANGLE_LB,STAND_ANGLE_RB);

    lf_.speed = 10;
    rf_.speed = 10;
    lb_.speed = 10;
    rb_.speed = 10;

    xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
}

void PetDog::scratching()       // 抓挠
{
    lf_.speed = 20;
    rf_.speed = 20;
    lb_.speed = 20;
    rb_.speed = 20;

    to_any_angle_task(STAND_ANGLE_LF,STAND_ANGLE_RF,STAND_ANGLE_LB,STAND_ANGLE_RB);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    to_any_angle_task(90,180,0,0);
    vTaskDelay(1200 / portTICK_PERIOD_MS);
    int wave_num = 5;
    while(wave_num -- != 0)
    {
        if (xEventGroupGetBits(action_task_event_) & STOP_TASK_EVENT)
        {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
        for (size_t i = 0; i < 45; i+=4)
        {
            set_left_back_angle(i);
            vTaskDelay(15 / portTICK_PERIOD_MS);
        }
        for (size_t i = 44; i > 0; i-=4)
        {
            set_left_back_angle(i);
            vTaskDelay(25 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    stand();

    lf_.speed = 10;
    rf_.speed = 10;
    lb_.speed = 10;
    rb_.speed = 10;
}

void PetDog::walkfront()
{
    while(1)
    {
        if (!WalkFrontCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::walkfrontFour()
{
    for (int i = 0; i < 4; ++i) {
        if (!WalkFrontCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::walkBack()
{
    while(1)
    {
        if (!WalkBackCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::turnLeft()
{
    while(1)
    {
        if (!TurnLeftCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::turnLeft90()
{
    constexpr int kTurn90Cycles = 2;
    for (int i = 0; i < kTurn90Cycles; ++i) {
        if (!TurnLeftCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::turnRight()
{
    while(1)
    {
        if (!TurnRightCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::turnRight90()
{
    constexpr int kTurn90Cycles = 2;
    for (int i = 0; i < kTurn90Cycles; ++i) {
        if (!TurnRightCycle()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
    }
    stand();
}

void PetDog::stop()
{
    xEventGroupSetBits(action_task_event_,STOP_TASK_EVENT);
}

void PetDog::petwave()          //挥手
{
    to_any_angle_task(90,90,50,0);
    vTaskDelay(600 / portTICK_PERIOD_MS);
    int wave_num = 5;
    while(wave_num -- != 0)
    {
        for (size_t i = 0; i < 65; i+=4)
        {
            set_left_front_angle(i);
            vTaskDelay(25 / portTICK_PERIOD_MS);
        }
        for (size_t i = 64; i > 0; i-=4)
        {
            set_left_front_angle(i);
            vTaskDelay(25 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    stand();
}

void PetDog::swayFrontBack()
{
    to_any_angle_task(STAND_ANGLE_LF, STAND_ANGLE_RF, STAND_ANGLE_LB, STAND_ANGLE_RB);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    for (int i = 0; i < 4; ++i) {
        if (should_stop()) {
            xEventGroupClearBits(action_task_event_,STOP_TASK_EVENT);
            break;
        }
        to_any_angle_task(60, 60, 120, 120);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        to_any_angle_task(120, 120, 60, 60);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    stand();
}

void PetDog::Action(int  action)
{
    // Long-running motions are started in independent tasks so caller returns fast.
    // stop()/state changes are used to interrupt cyclic motions cooperatively.
    switch(action)
    {
        case kActionStateTurnLeft:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->turnLeft();
                vTaskDelete(NULL);
            },"TurnLeft",2048,this,1,NULL);
            break;
        case kActionStateTurnRight:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->turnRight();
                vTaskDelete(NULL);
            },"TurnRight",2048,this,1,NULL);
            break;
        case kActionStateWalk:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->walkfront();
                vTaskDelete(NULL);
            },"walkfront",2048,this,1,NULL);
            break;
        case kActionStateWalkFour:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->walkfrontFour();
                vTaskDelete(NULL);
            },"walkfront_four",2048,this,1,NULL);
            break;
        case kActionStateWalkBack:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->walkBack();
                vTaskDelete(NULL);
            },"walkBack",2048,this,1,NULL);
            break;
        case kActionStateTurnLeft90:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->turnLeft90();
                vTaskDelete(NULL);
            },"turnLeft90",2048,this,1,NULL);
            break;
        case kActionStateTurnRight90:
            xTaskCreate([](void* arg)
            {
                auto this_ = (PetDog*)arg;
                this_->turnRight90();
                vTaskDelete(NULL);
            },"turnRight90",2048,this,1,NULL);
            break;
        case kActionStateSleep:
            stop();
            petsleep();
            break;
        case kActionStateStand:
            stop();
            stand();
            break;
        case kActionStateSitdown:
            stop();
            sitdown();
            break;
        case kActionStateStop:
            stop();
            break;
        case kActionStateWave:
            stop();
            petwave();
            break;
        case kActionStateSway:
            stop();
            swayFrontBack();
            break;
        case kActionStateIdleRandom:
            stop();
            idle_activate(rand() % 4);
            break;
    }
}

void PetDog::to_any_angle_task(uint8_t lf_angle,uint8_t rf_angle,uint8_t lb_angle,uint8_t rb_angle)
{
    lf_.angle = lf_angle;
    rf_.angle = rf_angle;
    lb_.angle = lb_angle;
    rb_.angle = rb_angle;

    xTaskCreate([](void* arg)
    {
        auto this_ = (PetDog*)arg;
        this_->to_tar_angle(&this_->lf_);
        vTaskDelete(NULL);
    }, "to_tar_angle", 2048, this, 1, NULL);

    xTaskCreate([](void* arg)
    {
        auto this_ = (PetDog*)arg;
        this_->to_tar_angle(&this_->rf_);
        vTaskDelete(NULL);
    }, "to_tar_angle", 2048, this, 1, NULL);

    xTaskCreate([](void* arg)
    {
        auto this_ = (PetDog*)arg;
        this_->to_tar_angle(&this_->lb_);
        vTaskDelete(NULL);
    }, "to_tar_angle", 2048, this, 1, NULL);

    xTaskCreate([](void* arg)
    {
        auto this_ = (PetDog*)arg;
        this_->to_tar_angle(&this_->rb_);
        vTaskDelete(NULL);
    }, "to_tar_angle", 2048, this, 1, NULL);
    vTaskDelay(500 / portTICK_PERIOD_MS);       //很重要
    xEventGroupSetBits(action_task_event_,START_TASK_EVENT);
    
}

void PetDog::to_tar_angle(target_angle_config_t *tar)
{
    if (tar == NULL)    
    {
        return;
    }
    int speed = tar->speed;
    
    if(xEventGroupWaitBits(action_task_event_,START_TASK_EVENT,pdTRUE,pdTRUE,portMAX_DELAY))
    {
        if(tar->index == LEG1)
        {
            if(left_front_angle > tar->angle)
            {
                for (size_t i = left_front_angle; i > tar->angle; i--)
                {   
                    set_left_front_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }else if (left_front_angle < tar->angle)
            {   
                for (size_t i = left_front_angle; i < tar->angle; i++)
                {
                    set_left_front_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }
        }else if(tar->index == LEG2)
        {
            if(right_front_angle > tar->angle)
            {
                for (size_t i = right_front_angle; i > tar->angle; i--)
                {   
                    set_right_front_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }else if (right_front_angle < tar->angle)
            {   
                for (size_t i = right_front_angle; i < tar->angle; i++)
                {
                    set_right_front_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }
        }else if(tar->index == LEG3)
        {
            if(left_back_angle > tar->angle)
            {
                for (size_t i = left_back_angle; i > tar->angle; i--)
                {   
                    set_left_back_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }else if (left_back_angle < tar->angle)
            {
                for (size_t i = left_back_angle; i < tar->angle; i++)
                {
                    set_left_back_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }
        }else if(tar->index == LEG4)
        {
            if(right_back_angle > tar->angle)
            {
                for (size_t i = right_back_angle; i > tar->angle; i--)
                {   
                    set_right_back_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }else if (right_back_angle < tar->angle)
            {
                for (size_t i = right_back_angle; i < tar->angle; i++)
                {
                    set_right_back_angle(i);
                    vTaskDelay(speed / portTICK_PERIOD_MS); 
                }
            }
        }
        printf("leg=%d,angle=%d\r\n",tar->index,tar->angle);
    }
}


void PetDog::set_right_back_angle(int angle)
{
    int anglex = 180 - angle;
    right_back_angle = angle;
    ledc_set_duty(LEDC_MODE, CHANNEL_0, anglex * per_angle + LEDC_MIN_DUTY);
    ledc_update_duty(LEDC_MODE, CHANNEL_0);
}
void PetDog::set_left_front_angle(int angle)
{
    left_front_angle = angle;
    ledc_set_duty(LEDC_MODE, CHANNEL_1, angle * per_angle + LEDC_MIN_DUTY);
    ledc_update_duty(LEDC_MODE, CHANNEL_1);
}
void PetDog::set_right_front_angle(int angle)
{
    int anglex = 180 - angle;
    right_front_angle = angle;
    ledc_set_duty(LEDC_MODE, CHANNEL_2, anglex * per_angle + LEDC_MIN_DUTY);
    ledc_update_duty(LEDC_MODE, CHANNEL_2);
}
void PetDog::set_left_back_angle(int angle)
{
    left_back_angle = angle;
    ledc_set_duty(LEDC_MODE, CHANNEL_3, angle * per_angle + LEDC_MIN_DUTY);
    ledc_update_duty(LEDC_MODE, CHANNEL_3);
}
void PetDog::set_angle(uint8_t lf_angle,uint8_t rf_angle,uint8_t lb_angle,uint8_t rb_angle)
{
    set_left_front_angle(lf_angle);
    vTaskDelay(40 / portTICK_PERIOD_MS);
    set_right_front_angle(rf_angle);
    vTaskDelay(40 / portTICK_PERIOD_MS);
    set_left_back_angle(lb_angle);
    vTaskDelay(40 / portTICK_PERIOD_MS);
    set_right_back_angle(rb_angle);
    vTaskDelay(40 / portTICK_PERIOD_MS);
}

