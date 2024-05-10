#include "wx/config/option.h"

#include <cstring>

#include <variant.hpp>

#include <wx/log.h>
#include <wx/translation.h>

#define VBAM_OPTION_INTERNAL_INCLUDE
#include "wx/config/internal/option-internal.h"
#undef VBAM_OPTION_INTERNAL_INCLUDE

#include "core/base/check.h"
#include "wx/config/option-proxy.h"

namespace config {

// static
Option* Option::ByName(const wxString& config_name) {
    nonstd::optional<OptionID> option_id = internal::StringToOptionId(config_name);
    if (!option_id) {
        return nullptr;
    }
    return ByID(option_id.value());
}

// static
Option* Option::ByID(OptionID id) {
    VBAM_CHECK(id != OptionID::Last);
    return &All()[static_cast<size_t>(id)];
}

Option::~Option() = default;

Option::Observer::Observer(OptionID option_id) : option_(Option::ByID(option_id)) {
    VBAM_CHECK(option_);
    option_->AddObserver(this);
}
Option::Observer::~Observer() {
    option_->RemoveObserver(this);
}

Option::Option(OptionID id)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(),
      min_(),
      max_() {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_none());
}

Option::Option(OptionID id, bool* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_() {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_bool());
}

Option::Option(OptionID id, double* option, double min, double max)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(min),
      max_(max) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_double());

    // Validate the initial value.
    SetDouble(*option);
}

Option::Option(OptionID id, int32_t* option, int32_t min, int32_t max)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(min),
      max_(max) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_int());

    // Validate the initial value.
    SetInt(*option);
}

Option::Option(OptionID id, uint32_t* option, uint32_t min, uint32_t max)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(min),
      max_(max) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_unsigned());

    // Validate the initial value.
    SetUnsigned(*option);
}

Option::Option(OptionID id, wxString* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_() {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_string());
}

Option::Option(OptionID id, Filter* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_(nonstd::in_place_type<size_t>, internal::MaxForType(type_)) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_filter());
}

Option::Option(OptionID id, Interframe* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_(nonstd::in_place_type<size_t>, internal::MaxForType(type_)) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_interframe());
}

Option::Option(OptionID id, RenderMethod* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_(nonstd::in_place_type<size_t>, internal::MaxForType(type_)) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_render_method());
}

Option::Option(OptionID id, AudioApi* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_(nonstd::in_place_type<size_t>, internal::MaxForType(type_)) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_audio_api());
}

Option::Option(OptionID id, AudioRate* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_(nonstd::in_place_type<size_t>, internal::MaxForType(type_)) {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_audio_rate());
}

Option::Option(OptionID id, uint16_t* option)
    : id_(id),
      config_name_(internal::kAllOptionsData[static_cast<size_t>(id)].config_name),
      command_(internal::kAllOptionsData[static_cast<size_t>(id)].command),
      ux_helper_(wxGetTranslation(internal::kAllOptionsData[static_cast<size_t>(id)].ux_helper)),
      type_(kOptionsTypes[static_cast<size_t>(id)]),
      value_(option),
      min_(),
      max_() {
    VBAM_CHECK(id != OptionID::Last);
    VBAM_CHECK(is_gb_palette());
}

bool Option::GetBool() const {
    VBAM_CHECK(is_bool());
    return *(nonstd::get<bool*>(value_));
}

double Option::GetDouble() const {
    VBAM_CHECK(is_double());
    return *(nonstd::get<double*>(value_));
}

int32_t Option::GetInt() const {
    VBAM_CHECK(is_int());
    return *(nonstd::get<int32_t*>(value_));
}

uint32_t Option::GetUnsigned() const {
    VBAM_CHECK(is_unsigned());
    return *(nonstd::get<uint32_t*>(value_));
}

const wxString& Option::GetString() const {
    VBAM_CHECK(is_string());
    return *(nonstd::get<wxString*>(value_));
}

Filter Option::GetFilter() const {
    VBAM_CHECK(is_filter());
    return *(nonstd::get<Filter*>(value_));
}

Interframe Option::GetInterframe() const {
    VBAM_CHECK(is_interframe());
    return *(nonstd::get<Interframe*>(value_));
}

RenderMethod Option::GetRenderMethod() const {
    VBAM_CHECK(is_render_method());
    return *(nonstd::get<RenderMethod*>(value_));
}

AudioApi Option::GetAudioApi() const {
    VBAM_CHECK(is_audio_api());
    return *(nonstd::get<AudioApi*>(value_));
}

AudioRate Option::GetAudioRate() const {
    VBAM_CHECK(is_audio_rate());
    return *(nonstd::get<AudioRate*>(value_));
}

wxString Option::GetEnumString() const {
    switch (type_) {
        case Option::Type::kFilter:
            return internal::FilterToString(GetFilter());
        case Option::Type::kInterframe:
            return internal::InterframeToString(GetInterframe());
        case Option::Type::kRenderMethod:
            return internal::RenderMethodToString(GetRenderMethod());
        case Option::Type::kAudioApi:
            return internal::AudioApiToString(GetAudioApi());
        case Option::Type::kAudioRate:
            return internal::AudioRateToString(GetAudioRate());

        // We don't use default here to explicitly trigger a compiler warning
        // when adding a new value.
        case Option::Type::kNone:
        case Option::Type::kBool:
        case Option::Type::kDouble:
        case Option::Type::kInt:
        case Option::Type::kUnsigned:
        case Option::Type::kString:
        case Option::Type::kGbPalette:
            VBAM_CHECK(false);
            return wxEmptyString;
    }
    VBAM_NOTREACHED();
    return wxEmptyString;
}

std::array<uint16_t, 8> Option::GetGbPalette() const {
    VBAM_CHECK(is_gb_palette());

    const uint16_t* raw_palette = (nonstd::get<uint16_t*>(value_));
    std::array<uint16_t, 8> palette;
    std::memcpy(palette.data(), raw_palette, sizeof(palette));
    return palette;
}

wxString Option::GetGbPaletteString() const {
    VBAM_CHECK(is_gb_palette());

    wxString palette_string;
    uint16_t const* value = nonstd::get<uint16_t*>(value_);
    palette_string.Printf("%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X", value[0], value[1], value[2],
                          value[3], value[4], value[5], value[6], value[7]);
    return palette_string;
}

bool Option::SetBool(bool value) {
    VBAM_CHECK(is_bool());
    bool old_value = GetBool();
    *nonstd::get<bool*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetDouble(double value) {
    VBAM_CHECK(is_double());
    double old_value = GetDouble();
    if (value < nonstd::get<double>(min_) || value > nonstd::get<double>(max_)) {
        wxLogWarning(_("Invalid value %f for option %s; valid values are %f - %f"), value,
                     config_name_, nonstd::get<double>(min_), nonstd::get<double>(max_));
        return false;
    }
    *nonstd::get<double*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetInt(int32_t value) {
    VBAM_CHECK(is_int());
    int old_value = GetInt();
    if (value < nonstd::get<int32_t>(min_) || value > nonstd::get<int32_t>(max_)) {
        wxLogWarning(_("Invalid value %d for option %s; valid values are %d - %d"), value,
                     config_name_, nonstd::get<int32_t>(min_), nonstd::get<int32_t>(max_));
        return false;
    }
    *nonstd::get<int32_t*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetUnsigned(uint32_t value) {
    VBAM_CHECK(is_unsigned());
    uint32_t old_value = GetUnsigned();
    if (value < nonstd::get<uint32_t>(min_) || value > nonstd::get<uint32_t>(max_)) {
        wxLogWarning(_("Invalid value %d for option %s; valid values are %d - %d"), value,
                     config_name_, nonstd::get<uint32_t>(min_), nonstd::get<uint32_t>(max_));
        return false;
    }
    *nonstd::get<uint32_t*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetString(const wxString& value) {
    VBAM_CHECK(is_string());
    const wxString old_value = GetString();
    *nonstd::get<wxString*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetFilter(const Filter& value) {
    VBAM_CHECK(is_filter());
    VBAM_CHECK(value < Filter::kLast);
    const Filter old_value = GetFilter();
    *nonstd::get<Filter*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetInterframe(const Interframe& value) {
    VBAM_CHECK(is_interframe());
    VBAM_CHECK(value < Interframe::kLast);
    const Interframe old_value = GetInterframe();
    *nonstd::get<Interframe*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetRenderMethod(const RenderMethod& value) {
    VBAM_CHECK(is_render_method());
    VBAM_CHECK(value < RenderMethod::kLast);
    const RenderMethod old_value = GetRenderMethod();
    *nonstd::get<RenderMethod*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetAudioApi(const AudioApi& value) {
    VBAM_CHECK(is_audio_api());
    VBAM_CHECK(value < AudioApi::kLast);
    const AudioApi old_value = GetAudioApi();
    *nonstd::get<AudioApi*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetAudioRate(const AudioRate& value) {
    VBAM_CHECK(is_audio_rate());
    VBAM_CHECK(value < AudioRate::kLast);
    const AudioRate old_value = GetAudioRate();
    *nonstd::get<AudioRate*>(value_) = value;
    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetEnumString(const wxString& value) {
    switch (type_) {
        case Option::Type::kFilter:
            return SetFilter(internal::StringToFilter(config_name_, value));
        case Option::Type::kInterframe:
            return SetInterframe(internal::StringToInterframe(config_name_, value));
        case Option::Type::kRenderMethod:
            return SetRenderMethod(internal::StringToRenderMethod(config_name_, value));
        case Option::Type::kAudioApi:
            return SetAudioApi(internal::StringToAudioApi(config_name_, value));
        case Option::Type::kAudioRate:
            return SetAudioRate(internal::StringToSoundQuality(config_name_, value));

        // We don't use default here to explicitly trigger a compiler warning
        // when adding a new value.
        case Option::Type::kNone:
        case Option::Type::kBool:
        case Option::Type::kDouble:
        case Option::Type::kInt:
        case Option::Type::kUnsigned:
        case Option::Type::kString:
        case Option::Type::kGbPalette:
            VBAM_CHECK(false);
            return false;
    }
    VBAM_NOTREACHED();
    return false;
}

bool Option::SetGbPalette(const std::array<uint16_t, 8>& value) {
    VBAM_CHECK(is_gb_palette());

    uint16_t* dest = nonstd::get<uint16_t*>(value_);

    // Keep a copy of the current value.
    std::array<uint16_t, 8> old_value;
    std::copy(dest, dest + 8, old_value.data());

    // Set the new value.
    std::copy(value.begin(), value.end(), dest);

    if (old_value != value) {
        CallObservers();
    }
    return true;
}

bool Option::SetGbPaletteString(const wxString& value) {
    VBAM_CHECK(is_gb_palette());

    // 8 values of 4 chars and 7 commas.
    static constexpr size_t kPaletteStringSize = 8 * 4 + 7;

    if (value.size() != kPaletteStringSize) {
        wxLogWarning(_("Invalid value %s for option %s"), value, config_name_);
        return false;
    }

    std::array<uint16_t, 8> new_value;
    for (size_t i = 0; i < 8; i++) {
        wxString number = value.substr(i * 5, 4);
        long temp = 0;
        if (number.ToLong(&temp, 16)) {
            new_value[i] = temp;
        } else {
            wxLogWarning(_("Invalid value %s for option %s"), value, config_name_);
            return false;
        }
    }

    return SetGbPalette(new_value);
}

double Option::GetDoubleMin() const {
    VBAM_CHECK(is_double());
    return nonstd::get<double>(min_);
}

double Option::GetDoubleMax() const {
    VBAM_CHECK(is_double());
    return nonstd::get<double>(max_);
}

int32_t Option::GetIntMin() const {
    VBAM_CHECK(is_int());
    return nonstd::get<int32_t>(min_);
}

int32_t Option::GetIntMax() const {
    VBAM_CHECK(is_int());
    return nonstd::get<int32_t>(max_);
}

uint32_t Option::GetUnsignedMin() const {
    VBAM_CHECK(is_unsigned());
    return nonstd::get<uint32_t>(min_);
}

uint32_t Option::GetUnsignedMax() const {
    VBAM_CHECK(is_unsigned());
    return nonstd::get<uint32_t>(max_);
}

size_t Option::GetEnumMax() const {
    VBAM_CHECK(is_filter() || is_interframe() || is_render_method() || is_audio_api() ||
           is_audio_rate());
    return nonstd::get<size_t>(max_);
}

void Option::NextFilter() {
    VBAM_CHECK(is_filter());
    const int old_value = static_cast<int>(GetFilter());
    const int new_value = (old_value + 1) % kNbFilters;
    SetFilter(static_cast<Filter>(new_value));
}

void Option::NextInterframe() {
    VBAM_CHECK(is_interframe());
    const int old_value = static_cast<int>(GetInterframe());
    const int new_value = (old_value + 1) % kNbInterframes;
    SetInterframe(static_cast<Interframe>(new_value));
}

wxString Option::ToHelperString() const {
    wxString helper_string = config_name_;

    switch (type_) {
        case Option::Type::kNone:
            break;
        case Option::Type::kBool:
            helper_string.Append(" (flag)");
            break;
        case Option::Type::kDouble:
            helper_string.Append(" (decimal)");
            break;
        case Option::Type::kInt:
            helper_string.Append(" (int)");
            break;
        case Option::Type::kUnsigned:
            helper_string.Append(" (unsigned)");
            break;
        case Option::Type::kString:
            helper_string.Append(" (string)");
            break;
        case Option::Type::kFilter:
        case Option::Type::kInterframe:
        case Option::Type::kRenderMethod:
        case Option::Type::kAudioApi:
        case Option::Type::kAudioRate:
            helper_string.Append(" (");
            helper_string.Append(internal::AllEnumValuesForType(type_));
            helper_string.Append(")");
            break;
        case Option::Type::kGbPalette:
            helper_string.Append(" (XXXX,XXXX,XXXX,XXXX,XXXX,XXXX,XXXX,XXXX)");
            break;
    }
    helper_string.Append("\n\t");
    helper_string.Append(ux_helper_);
    helper_string.Append("\n");

    return helper_string;
}

void Option::AddObserver(Observer* observer) {
    VBAM_CHECK(observer);
    [[maybe_unused]] const auto pair = observers_.emplace(observer);
    VBAM_CHECK(pair.second);
}

void Option::RemoveObserver(Observer* observer) {
    VBAM_CHECK(observer);
    [[maybe_unused]] const size_t removed = observers_.erase(observer);
    VBAM_CHECK(removed == 1u);
}

void Option::CallObservers() {
    VBAM_CHECK(!calling_observers_);
    calling_observers_ = true;
    for (const auto observer : observers_) {
        observer->OnValueChanged();
    }
    calling_observers_ = false;
}

}  // namespace config
