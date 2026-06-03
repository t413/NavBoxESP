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

template<typename T>T parseValue(const SetValue& v) { return static_cast<T>(v.toInt()); } // Fallback parser: integer-like types
template<> float parseValue<float>(const SetValue& v) { return v.toFloat(); } // float specialization
template<> bool parseValue<bool>(const SetValue& v) { return (v.toInt() != 0 || v == "true"); } // bool specialization

template<typename T>
void ScalarSetting<T>::set(const SetValue& value) {
    T newVal = parseValue<T>(value);
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

SettingsManager::Group SettingsManager::group(std::string name) {
    for (size_t i = 0; i < groups_.size(); ++i)
        if (groups_[i] == name)
            return Group(i, this);
    groups_.push_back(name);
    MAP_LOG("Added group[%d] %s", (int)groups_.size() - 1, name.c_str());
    return Group(groups_.size() - 1, this);
}


template<typename T>
Setting& SettingsManager::Group::add(const SetValue& name, T* value, T min, T max) {
    auto setting = std::unique_ptr<ScalarSetting<T>>(new ScalarSetting<T>(name, value, min, max));
    setting->group_ = id;
    auto* ptr = setting.get();
    mgr->settings_.push_back(std::move(setting));
    return *ptr;
}

// Explicit template instantiations for SettingsManager::add
template Setting& SettingsManager::Group::add<int>(const SetValue& name, int* value, int min, int max);
template Setting& SettingsManager::Group::add<float>(const SetValue& name, float* value, float min, float max);
template Setting& SettingsManager::Group::add<uint8_t>(const SetValue& name, uint8_t* value, uint8_t min, uint8_t max);
template Setting& SettingsManager::Group::add<bool>(const SetValue& name, bool* value, bool min, bool max);

Setting& SettingsManager::Group::add(const SetValue& name, String* value) {
    auto setting = std::unique_ptr<StrSetting>(new StrSetting(name, value));
    setting->group_ = id;
    auto* ptr = setting.get();
    mgr->settings_.push_back(std::move(setting));
    return *ptr;
}

Setting& SettingsManager::Group::addFn(const SetValue& name, GetterFn getter, SetterFn setter) {
    auto setting = std::unique_ptr<CallbackSetting>(new CallbackSetting(name, getter, setter));
    setting->group_ = id;
    auto* ptr = setting.get();
    mgr->settings_.push_back(std::move(setting));
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

bool SettingsManager::load(bool lateOnly) {
    MAP_LOG("Setting load from %s into %d slots", configPath_.c_str(), (int)settings_.size());
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
            if (s->late_ != lateOnly) continue;
            JsonVariant v = kv.value();
            if        (v.is<float>()) { s->set(SetValue(v.as<float>()));
            } else if (v.is<int>())   { s->set(SetValue(v.as<int>()));
            } else { s->set(v.as<const char*>()); } //string
            const auto res = s->get();
            MAP_LOG("setting loaded %s -> %s", kv.key().c_str(), res.c_str());
        } else {
            MAP_LOG("setting missing from config: %s", kv.key().c_str());
        }
    }
    return true;
}

bool SettingsManager::save() {
    JsonDocument doc;
    // Load existing file to preserve unknown keys
    if (auto fileRead = SD.open(configPath_.c_str(), "r")) {
        deserializeJson(doc, fileRead);
        fileRead.close();
    }
    // Update or add current settings
    for (const auto& setting : settings_) {
        const SetValue& key = setting->getKey();
        SetValue value = setting->get();
        if (setting->isNum_) {
            doc[key.c_str()] = value.toFloat();
        } else {
            doc[key.c_str()] = value.c_str();
        }
    }
    if (auto file = SD.open(configPath_.c_str(), "w")) {
        serializeJsonPretty(doc, file);
        file.close();
        MAP_LOG("Saved %d settings", (int)settings_.size());
        return true;
    }
    MAP_LOG("Failed to open %s for writing", configPath_.c_str());
    return false;
}
