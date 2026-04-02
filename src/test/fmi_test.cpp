#include "cosmo/fmi/model_description.hpp"
#include "cosmo/fmi/fmi_types.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

using namespace cosmo::test;
using namespace cosmo::fmi;

TEST_CASE(model_description_basic)
{
    auto model_desc = create_model_description();
    
    REQUIRE(model_desc != nullptr);
    
    // Test setters and getters
    model_desc->set_model_name("TestModel");
    model_desc->set_guid("12345678-1234-1234-1234-123456789012");
    model_desc->set_version(fmi_version::fmi_2_0);
    model_desc->set_description("A test model");
    
    REQUIRE_EQ(model_desc->get_model_name(), "TestModel");
    REQUIRE_EQ(model_desc->get_guid(), "12345678-1234-1234-1234-123456789012");
    REQUIRE_EQ(model_desc->get_version(), fmi_version::fmi_2_0);
    REQUIRE_EQ(model_desc->get_description(), "A test model");
}

TEST_CASE(variable_management)
{
    auto model_desc = create_model_description();
    
    // Add variables
    variable_description var1;
    var1.name = "velocity";
    var1.vr = 1;
    var1.type = variable_type::real;
    var1.causality_type = causality::output;
    var1.variability_type = variability::continuous;
    var1.default_real = 0.0;
    
    variable_description var2;
    var2.name = "position";
    var2.vr = 2;
    var2.type = variable_type::real;
    var2.causality_type = causality::output;
    var2.variability_type = variability::continuous;
    var2.default_real = 0.0;
    
    variable_description var3;
    var3.name = "mass";
    var3.vr = 3;
    var3.type = variable_type::real;
    var3.causality_type = causality::parameter;
    var3.variability_type = variability::fixed;
    var3.default_real = 1.0;
    
    model_desc->add_variable(var1);
    model_desc->add_variable(var2);
    model_desc->add_variable(var3);
    
    // Test variable retrieval by name
    const variable_description* retrieved_var = model_desc->get_variable_by_name("velocity");
    REQUIRE(retrieved_var != nullptr);
    REQUIRE_EQ(retrieved_var->name, "velocity");
    REQUIRE_EQ(retrieved_var->vr, static_cast<value_reference>(1));
    
    // Test variable retrieval by VR
    retrieved_var = model_desc->get_variable_by_vr(2);
    REQUIRE(retrieved_var != nullptr);
    REQUIRE_EQ(retrieved_var->name, "position");
    
    // Test VR lookup by name
    value_reference vr = model_desc->get_vr_by_name("mass");
    REQUIRE_EQ(vr, static_cast<value_reference>(3));
    
    // Test name lookup by VR
    std::string name = model_desc->get_name_by_vr(1);
    REQUIRE_EQ(name, "velocity");
    
    // Test filtering by causality
    auto output_vars = model_desc->get_variables_by_causality(causality::output);
    REQUIRE_EQ(output_vars.size(), 2);
    
    auto param_vars = model_desc->get_variables_by_causality(causality::parameter);
    REQUIRE_EQ(param_vars.size(), 1);
    
    // Test filtering by type
    auto real_vars = model_desc->get_variables_by_type(variable_type::real);
    REQUIRE_EQ(real_vars.size(), 3);
}

int main()
{
    cosmo::log::info("Starting FMI test");
    return RUN_ALL_TESTS();
}