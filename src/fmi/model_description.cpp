#include "cosmo/fmi/model_description.hpp"
#include "cosmo/logger/logger.hpp"

#include <algorithm>

namespace cosmo {
namespace fmi {

model_description::model_description()
    : model_name("")
    , guid("")
    , version(fmi_version::fmi_2_0)
    , description("")
{
}

void model_description::add_variable(const variable_description& var)
{
    variables.push_back(var);
    variable_name_to_vr[var.name] = var.vr;
}

const variable_description* model_description::get_variable_by_name(const std::string& name) const
{
    auto it = variable_name_to_vr.find(name);
    if (it != variable_name_to_vr.end()) {
        value_reference vr = it->second;
        // Find the variable with this VR
        auto var_it = std::find_if(variables.begin(), variables.end(),
            [vr](const variable_description& var) {
                return var.vr == vr;
            });
        if (var_it != variables.end()) {
            return &(*var_it);
        }
    }
    return nullptr;
}

const variable_description* model_description::get_variable_by_vr(value_reference vr) const
{
    auto it = std::find_if(variables.begin(), variables.end(),
        [vr](const variable_description& var) {
            return var.vr == vr;
        });
    if (it != variables.end()) {
        return &(*it);
    }
    return nullptr;
}

value_reference model_description::get_vr_by_name(const std::string& name) const
{
    auto it = variable_name_to_vr.find(name);
    if (it != variable_name_to_vr.end()) {
        return it->second;
    }
    return static_cast<value_reference>(-1); // Invalid VR
}

std::string model_description::get_name_by_vr(value_reference vr) const
{
    const variable_description* var = get_variable_by_vr(vr);
    if (var) {
        return var->name;
    }
    return ""; // Empty string for invalid VR
}

std::vector<const variable_description*> model_description::get_variables_by_causality(causality c) const
{
    std::vector<const variable_description*> result;
    for (const auto& var : variables) {
        if (var.causality_type == c) {
            result.push_back(&var);
        }
    }
    return result;
}

std::vector<const variable_description*> model_description::get_variables_by_type(variable_type t) const
{
    std::vector<const variable_description*> result;
    for (const auto& var : variables) {
        if (var.type == t) {
            result.push_back(&var);
        }
    }
    return result;
}

std::unique_ptr<model_description> create_model_description()
{
    return std::unique_ptr<model_description>(new model_description());
}

} // namespace fmi
} // namespace cosmo