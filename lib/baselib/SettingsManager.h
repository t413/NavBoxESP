#pragma once
#include "stringmoc.h"
#include <vector>
#include <functional>
#include <memory>

// Universal value type for get/set—easily swappable later
using SetValue = String;
typedef std::function<SetValue()> GetterFn;
typedef std::function<void(const SetValue&)> SetterFn;


class Setting {
public:
    virtual ~Setting() = default;
    virtual SetValue get() const = 0;
    virtual void set(const SetValue& value) = 0;
    const SetValue& getKey() const { return key_; }
    Setting& onChange(std::function<void()> cb) {
        onChange_ = cb;
        return *this;
    }
    virtual Setting& setAdjBump(float) { return *this; }
    virtual float getAdjBump() const { return 1.0f; }
    SetValue key_;
    bool isNum_ = false;
    std::function<void()> onChange_;
    friend class SettingsManager;
};


class SettingsManager {
    std::vector<std::unique_ptr<Setting>> settings_;
    SetValue configPath_;

public:
    explicit SettingsManager(const SetValue& configPath = "/maps/config.json");
    ~SettingsManager();

    template<typename T>
    Setting& add(const SetValue& name, T* value, T min, T max);
    Setting& add(const SetValue& name, String*);
    Setting& addFn(const SetValue& name, GetterFn getter, SetterFn setter);

    const std::vector<std::unique_ptr<Setting>>& getSettings() const { return settings_; }

    bool load(); /// Load settings from config file
    bool save(); /// Saves settings to config file
    Setting* find(const SetValue& name);

};
