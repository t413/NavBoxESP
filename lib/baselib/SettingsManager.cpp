#include "SettingsManager.h"
#include "SettingTypes.h"
#include <navboxlib/log.h>
#include <ArduinoJson.h>
#include <navboxlib/fileclass.h>

// ============================================================================
// ScalarSetting implementation
// ============================================================================

template<typename T>
ScalarSetting<T>::ScalarSetting(const SetValue& key, T* value, T min, T max)
    : value_(value), min_(min), max_(max) {
    key_ = key;
    isNum_ = true;
}

template<typename T>
SetValue ScalarSetting<T>::get() const {
    return SetValue(*value_);
}

template<typename T>
void ScalarSetting<T>::set(const SetValue& value) {
    T newVal;
    if constexpr (std::is_same_v<T, float>) {
        newVal = value.toFloat();
    } else if constexpr (std::is_same_v<T, bool>) {
        newVal = (value.toInt() != 0 || value == "true");
    } else {
        newVal = value.toInt();
    }
    if (newVal >= min_ && newVal <= max_) { //apply limits
        if (*value_ != newVal) {
            *value_ = newVal;
            if (onChange_) onChange_();
        }
    }
}

// Explicit template instantiations
template class ScalarSetting<int>;
template class ScalarSetting<float>;
template class ScalarSetting<bool>;


// ============================================================================
// SettingsManager implementation
// ============================================================================

SettingsManager::SettingsManager(const SetValue& configPath)
    : configPath_(configPath) {
}

SettingsManager::~SettingsManager() {
}

template<typename T>
Setting& SettingsManager::add(const SetValue& name, T* value, T min, T max) {
    auto setting = std::make_unique<ScalarSetting<T>>(name, value, min, max);
    auto* ptr = setting.get();
    settings_.push_back(std::move(setting));
    return *ptr;
}

// Explicit template instantiations for SettingsManager::add
template Setting& SettingsManager::add<int>(const SetValue& name, int* value, int min, int max);
template Setting& SettingsManager::add<float>(const SetValue& name, float* value, float min, float max);
template Setting& SettingsManager::add<uint8_t>(const SetValue& name, uint8_t* value, uint8_t min, uint8_t max);
template Setting& SettingsManager::add<bool>(const SetValue& name, bool* value, bool min, bool max);

Setting& SettingsManager::add(const SetValue& name, String* value) {
    auto setting = std::make_unique<StrSetting>(name, value);
    auto* ptr = setting.get();
    settings_.push_back(std::move(setting));
    return *ptr;
}

Setting& SettingsManager::addFn(const SetValue& name, GetterFn getter, SetterFn setter) {
    auto setting = std::make_unique<CallbackSetting>(name, getter, setter);
    auto* ptr = setting.get();
    settings_.push_back(std::move(setting));
    return *ptr;
}

Setting* SettingsManager::find(const SetValue& name) {
    for (auto& setting : settings_) {
        if (setting->getKey() == name) {
            return setting.get();
        }
    }
    return nullptr;
}

bool SettingsManager::load() {
    MAP_LOG("Loading settings from %s", configPath_.c_str());
    auto file = SD.open(configPath_.c_str(), "r");
    if (!file) {
        MAP_LOG("Failed to open %s for reading", configPath_.c_str());
        return false;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        MAP_LOG("JSON deserialization failed: %s", error.c_str());
        return false;
    }

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        Setting* s = find(kv.key().c_str());
        if (s) {
            JsonVariant v = kv.value();
            if        (v.is<float>()) { s->set(v.as<float>());
            } else if (v.is<int>())   { s->set(v.as<int>());
            } else { s->set(v.as<const char*>()); } //string
        } else {
            MAP_LOG("Unknown setting in config: %s", kv.key().c_str());
        }
    }
    return true;
}

bool SettingsManager::save() {
    JsonDocument doc;

    for (const auto& setting : settings_) {
        const SetValue& key = setting->getKey();

        SetValue value = setting->get();
        if (setting->isNum_) {
            doc[key.c_str()] = value.toFloat();
        } else {
            doc[key.c_str()] = value.c_str(); //TODO allow for non-string values
        }
    }

    auto file = SD.open(configPath_.c_str(), "w");
    // my file mock needs these methods:
    // size_t write(uint8_t c) { return write(&c, 1); }
    // int read() { uint8_t r; return read(&r, 1) == 1? r : -1; }
    if (!file) {
        MAP_LOG("Failed to open %s for writing", configPath_.c_str());
        return false;
    }
    serializeJsonPretty(doc, file);
    file.close();

    MAP_LOG("Saved %d settings", (int)settings_.size());
    return true;
}
