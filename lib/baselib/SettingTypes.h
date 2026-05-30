#pragma once
#include "SettingsManager.h"

template<typename T>
class ScalarSetting : public Setting {
    T* value_;
    T min_;
    T max_;

public:
    ScalarSetting(const SetValue& key, T* value, T min, T max);
    ~ScalarSetting() = default;
    SetValue get() const override;
    void set(const SetValue& value) override;
    T getValue() const { return *value_; }
};
extern template class ScalarSetting<int>;
extern template class ScalarSetting<float>;
extern template class ScalarSetting<bool>;


class CallbackSetting : public Setting {
    GetterFn getter_;
    SetterFn setter_;
public:
    CallbackSetting(const SetValue& k, GetterFn g, SetterFn s) : getter_(g), setter_(s) { key_ = k; }
    ~CallbackSetting() = default;
    SetValue get() const override { return getter_(); }
    void set(const SetValue& value) override { setter_(value); }
};

class StrSetting : public Setting {
    String* value_;
public:
    StrSetting(const SetValue& key, String* value) : value_(value) { key_ = key; }
    ~StrSetting() = default;
    SetValue get() const override { return SetValue(*value_); }
    void set(const SetValue& value) override { *value_ = value; }
};
