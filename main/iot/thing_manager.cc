#include "thing_manager.h"

#include <esp_log.h>

#define TAG "ThingManager"

namespace iot {

void ThingManager::AddThing(Thing* thing) {
     if (thing != nullptr) {  // 关键修复：添加空指针检查
        things_.push_back(thing);
    } else {
        ESP_LOGE(TAG, "Attempt to add null thing");
    }
}

std::string ThingManager::GetDescriptorsJson() {
    // 添加空集合检查
    if (things_.empty()) {
        return "[]";
    }
    
    std::string json_str = "[";
    for (auto& thing : things_) {
        if (thing == nullptr) {  // 关键修复：跳过空指针
            continue;
        }
        json_str += thing->GetDescriptorJson() + ",";
    }
    
    if (json_str.back() == ',') {
        json_str.pop_back();
    }
    json_str += "]";
    return json_str;
}

bool ThingManager::GetStatesJson(std::string& json, bool delta) {
    if (!delta) {
        last_states_.clear();
    }
    bool changed = false;
    json = "[";
    // 枚举thing，获取每个thing的state，如果发生变化，则更新，保存到last_states_
    // 如果delta为true，则只返回变化的部分
    for (auto& thing : things_) {
        if (thing == nullptr) {  // 关键修复：跳过空指针
            continue;
        }
        std::string state = thing->GetStateJson();
        if (delta) {
            // 如果delta为true，则只返回变化的部分
            auto it = last_states_.find(thing->name());
            if (it != last_states_.end() && it->second == state) {
                continue;
            }
            changed = true;
            last_states_[thing->name()] = state;
        }
        json += state + ",";
    }
    if (json.back() == ',') {
        json.pop_back();
    }
    json += "]";
    return changed;
}

void ThingManager::Invoke(const cJSON* command) {
    if (command == nullptr) {  // 关键修复：检查空命令
        return;
    }
    
    auto name = cJSON_GetObjectItem(command, "name");
    if (name == nullptr || name->valuestring == nullptr) {  // 关键修复：检查空名称
        return;
    }
    
    for (auto& thing : things_) {
        if (thing == nullptr) {  // 关键修复：跳过空指针
            continue;
        }
        if (thing->name() == name->valuestring) {
            thing->Invoke(command);
            return;
        }
    }
}

} // namespace iot
