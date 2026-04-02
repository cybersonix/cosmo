#include "cosmo/fmi/fmu_processor.hpp"
#include "cosmo/util/xml_parser.hpp"
#include "cosmo/logger/logger.hpp"

#include <fstream>
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace cosmo {
namespace fmi {

fmu_processor::fmu_processor()
    : loaded_(false)
{
}

fmu_processor::~fmu_processor()
{
    cleanup();
}

bool fmu_processor::load_fmu(const std::string& fmu_path)
{
    if (!util::file_exists(fmu_path)) {
        cosmo::log::error("FMU file not found: " + fmu_path);
        return false;
    }

    fmu_path_ = fmu_path;
    
    // 创建临时目录
    extract_dir_ = util::parent_directory(fmu_path) + "/.cosmo_temp_" + 
                   util::file_name_without_extension(fmu_path);
    
    if (!util::create_directory(extract_dir_)) {
        cosmo::log::error("Failed to create temporary directory: " + extract_dir_);
        return false;
    }

    // 解压FMU文件
    if (!unzip_fmu(fmu_path, extract_dir_)) {
        cosmo::log::error("Failed to extract FMU file: " + fmu_path);
        cleanup();
        return false;
    }

    loaded_ = true;
    cosmo::log::info("Successfully loaded FMU: " + fmu_path);
    return true;
}

bool fmu_processor::extract_to_directory(const std::string& output_dir)
{
    if (!loaded_) {
        cosmo::log::error("No FMU loaded");
        return false;
    }

    return unzip_fmu(fmu_path_, output_dir);
}

std::vector<std::string> fmu_processor::get_entry_names() const
{
    std::vector<std::string> names;
    for (const auto& entry : entries_) {
        names.push_back(entry.name);
    }
    return names;
}

bool fmu_processor::get_entry_content(const std::string& entry_name, std::vector<unsigned char>& content)
{
    for (const auto& entry : entries_) {
        if (entry.name == entry_name) {
            if (entry.type == fmu_entry_type::directory) {
                cosmo::log::error("Cannot read content from directory: " + entry_name);
                return false;
            }
            
            // 如果数据已经解压，直接返回
            if (!entry.data.empty()) {
                content = entry.data;
                return true;
            }
            
            // 否则尝试从文件读取
            std::string full_path = extract_dir_ + "/" + entry_name;
            std::ifstream file(full_path, std::ios::binary);
            if (!file.is_open()) {
                cosmo::log::error("Failed to open file: " + full_path);
                return false;
            }
            
            file.seekg(0, std::ios::end);
            auto file_size_pos = file.tellg();
            if (file_size_pos <= 0) {
                cosmo::log::error("无法获取文件大小: " + full_path);
                return false;
            }
            size_t file_size = static_cast<size_t>(file_size_pos);
            file.seekg(0, std::ios::beg);
            
            content.resize(file_size);
            file.read(reinterpret_cast<char*>(content.data()), file_size);
            
            return true;
        }
    }
    
    cosmo::log::error("Entry not found: " + entry_name);
    return false;
}

std::unique_ptr<model_description> fmu_processor::parse_model_description()
{
    if (!loaded_) {
        cosmo::log::error("No FMU loaded");
        return nullptr;
    }

    // 读取modelDescription.xml
    std::string xml_path = extract_dir_ + "/modelDescription.xml";
    if (!util::file_exists(xml_path)) {
        cosmo::log::error("modelDescription.xml not found in FMU");
        return nullptr;
    }

    std::string xml_content = util::read_file(xml_path);
    if (xml_content.empty()) {
        cosmo::log::error("Failed to read modelDescription.xml");
        return nullptr;
    }

    return parse_model_description_xml(xml_content);
}

std::string fmu_processor::get_binary_path(const std::string& platform) const
{
    if (!loaded_) {
        return "";
    }

    std::string platform_name = platform.empty() ? get_current_platform() : platform;
    std::string binary_dir = extract_dir_ + "/binaries/" + platform_name;
    
    if (!util::directory_exists(binary_dir)) {
        cosmo::log::warn("Binary directory not found: " + binary_dir);
        return "";
    }

    // 查找动态库文件
    std::vector<std::string> files = util::list_files(binary_dir);
    for (const auto& file : files) {
        std::string ext = util::file_extension(file);
        if (ext == ".so" || ext == ".dll" || ext == ".dylib") {
            return binary_dir + "/" + file;
        }
    }

    return "";
}

void fmu_processor::cleanup()
{
    if (!extract_dir_.empty() && util::directory_exists(extract_dir_)) {
        // 删除临时目录：使用引号包裹路径防止命令注入
        // 进一步验证路径不含特殊字符
        bool safe_path = true;
        for (char c : extract_dir_) {
            if (c == '\'' || c == '"' || c == '`' || c == '$' || c == '\\' || c == '\n') {
                safe_path = false;
                break;
            }
        }
        if (!safe_path) {
            cosmo::log::error("临时目录路径包含不安全字符，跳过清理: " + extract_dir_);
        } else {
#ifdef _WIN32
            std::string cmd = "rmdir /s /q \"" + extract_dir_ + "\"";
#else
            std::string cmd = "rm -rf \"" + extract_dir_ + "\"";
#endif
            std::system(cmd.c_str());
        }
    }
    
    entries_.clear();
    loaded_ = false;
}

bool fmu_processor::unzip_fmu(const std::string& fmu_path, const std::string& output_dir)
{
    // 路径安全检查：防止命令注入
    for (const auto& path : {fmu_path, output_dir}) {
        for (char c : path) {
            if (c == '\'' || c == '"' || c == '`' || c == '$' || c == '\\' || c == '\n') {
                cosmo::log::error("FMU路径包含不安全字符: " + path);
                return false;
            }
        }
    }

    // 使用系统命令解压ZIP文件
#ifdef _WIN32
    std::string cmd = "powershell -command \"Expand-Archive -Path '" + fmu_path + 
                      "' -DestinationPath '" + output_dir + "' -Force\"";
#else
    std::string cmd = "unzip -o \"" + fmu_path + "\" -d \"" + output_dir + "\"";
#endif

    cosmo::log::info("解压FMU: " + fmu_path + " -> " + output_dir);
    
    int result = std::system(cmd.c_str());
    if (result != 0) {
        cosmo::log::error("Failed to extract FMU, command returned: " + std::to_string(result));
        return false;
    }

    // 扫描解压后的文件
    if (!util::directory_exists(output_dir)) {
        cosmo::log::error("Output directory does not exist after extraction");
        return false;
    }

    // 递归扫描文件
    std::function<void(const std::string&)> scan_directory = [&](const std::string& dir) {
        std::vector<std::string> items = util::list_files(dir);
        for (const auto& item : items) {
            std::string full_path = dir + "/" + item;
            std::string relative_path = full_path.substr(output_dir.length() + 1);
            
            if (util::directory_exists(full_path)) {
                fmu_entry entry;
                entry.name = relative_path;
                entry.type = fmu_entry_type::directory;
                entry.size = 0;
                entry.compressed_size = 0;
                entries_.push_back(entry);
                
                scan_directory(full_path);
            } else {
                fmu_entry entry;
                entry.name = relative_path;
                entry.type = fmu_entry_type::file;
                
                // 获取文件大小
                std::ifstream file(full_path, std::ios::binary | std::ios::ate);
                if (file.is_open()) {
                    entry.size = file.tellg();
                    entry.compressed_size = entry.size; // 已解压
                } else {
                    entry.size = 0;
                    entry.compressed_size = 0;
                }
                
                entries_.push_back(entry);
            }
        }
    };

    scan_directory(output_dir);
    
    cosmo::log::info("Extracted " + std::to_string(entries_.size()) + " entries from FMU");
    return true;
}

bool fmu_processor::parse_zip_header(const std::vector<unsigned char>& data, size_t& pos)
{
    // 检查ZIP本地文件头签名 (0x04034b50)
    if (pos + 4 > data.size()) {
        return false;
    }
    
    unsigned int signature = static_cast<unsigned int>(data[pos])
                            | (static_cast<unsigned int>(data[pos + 1]) << 8)
                            | (static_cast<unsigned int>(data[pos + 2]) << 16)
                            | (static_cast<unsigned int>(data[pos + 3]) << 24);
    
    if (signature != 0x04034b50) {
        return false;
    }
    
    pos += 4;
    return true;
}

bool fmu_processor::parse_central_directory(const std::vector<unsigned char>& /*data*/, size_t /*pos*/)
{
    // 这里可以实现完整的ZIP解析，但为了简化，我们使用系统命令
    return true;
}

bool fmu_processor::decompress_entry(const fmu_entry& /*entry*/, std::vector<unsigned char>& /*output*/)
{
    // 由于DEFLATE解压缩比较复杂，我们使用系统命令来处理
    // 在实际项目中，可以集成zlib或miniz库
    return false;
}

std::unique_ptr<model_description> fmu_processor::parse_model_description_xml(const std::string& xml_content)
{
    auto doc = util::create_xml_document();
    if (!doc->load_string(xml_content)) {
        cosmo::log::error("Failed to parse modelDescription.xml: " + doc->get_error());
        return nullptr;
    }

    auto root = doc->get_root();
    if (!root || root->get_name() != "fmiModelDescription") {
        cosmo::log::error("Invalid modelDescription.xml: missing fmiModelDescription root element");
        return nullptr;
    }

    auto model_desc = create_model_description();
    
    // 解析基本属性
    model_desc->set_model_name(root->get_attribute("modelName", "Unknown"));
    model_desc->set_guid(root->get_attribute("guid", ""));
    model_desc->set_description(root->get_attribute("description", ""));
    
    // 解析FMI版本
    std::string fmi_version_str = root->get_attribute("fmiVersion", "2.0");
    if (fmi_version_str.find("1.0") != std::string::npos) {
        model_desc->set_version(fmi_version::fmi_1_0);
    } else if (fmi_version_str.find("2.0") != std::string::npos) {
        model_desc->set_version(fmi_version::fmi_2_0);
    } else if (fmi_version_str.find("3.0") != std::string::npos) {
        model_desc->set_version(fmi_version::fmi_3_0);
    } else {
        model_desc->set_version(fmi_version::fmi_2_0); // 默认
    }
    
    // 解析变量
    auto model_variables = root->get_child("ModelVariables");
    if (model_variables) {
        auto scalar_variables = model_variables->get_children("ScalarVariable");
        for (const auto& sv : scalar_variables) {
            variable_description var;
            var.name = sv->get_attribute("name", "");
            var.description = sv->get_attribute("description", "");
            
            // 解析valueReference
            std::string vr_str = sv->get_attribute("valueReference", "0");
            try {
                var.vr = static_cast<value_reference>(std::stoi(vr_str));
            } catch (...) {
                var.vr = 0;
            }
            
            // 解析causality
            std::string causality_str = sv->get_attribute("causality", "local");
            if (causality_str == "parameter") {
                var.causality_type = causality::parameter;
            } else if (causality_str == "input") {
                var.causality_type = causality::input;
            } else if (causality_str == "output") {
                var.causality_type = causality::output;
            } else if (causality_str == "independent") {
                var.causality_type = causality::independent;
            } else {
                var.causality_type = causality::local;
            }
            
            // 解析variability
            std::string variability_str = sv->get_attribute("variability", "continuous");
            if (variability_str == "constant") {
                var.variability_type = variability::constant;
            } else if (variability_str == "fixed") {
                var.variability_type = variability::fixed;
            } else if (variability_str == "tunable") {
                var.variability_type = variability::tunable;
            } else if (variability_str == "discrete") {
                var.variability_type = variability::discrete;
            } else {
                var.variability_type = variability::continuous;
            }
            
            // 解析类型和默认值
            auto real_node = sv->get_child("Real");
            if (real_node) {
                var.type = variable_type::real;
                std::string start_str = real_node->get_attribute("start", "0.0");
                try {
                    var.default_real = std::stod(start_str);
                } catch (...) {
                    var.default_real = 0.0;
                }
            }
            
            auto integer_node = sv->get_child("Integer");
            if (integer_node) {
                var.type = variable_type::integer;
                std::string start_str = integer_node->get_attribute("start", "0");
                try {
                    var.default_integer = std::stoi(start_str);
                } catch (...) {
                    var.default_integer = 0;
                }
            }
            
            auto boolean_node = sv->get_child("Boolean");
            if (boolean_node) {
                var.type = variable_type::boolean;
                std::string start_str = boolean_node->get_attribute("start", "false");
                var.default_boolean = (start_str == "true" || start_str == "1");
            }
            
            auto string_node = sv->get_child("String");
            if (string_node) {
                var.type = variable_type::string;
                var.default_string = string_node->get_attribute("start", "");
            }
            
            model_desc->add_variable(var);
        }
    }
    
    return model_desc;
}

std::string fmu_processor::get_current_platform() const
{
#ifdef _WIN32
#ifdef _WIN64
    return "win64";
#else
    return "win32";
#endif
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_MAC
    return "darwin64";
#else
    return "darwin";
#endif
#elif __linux__
#ifdef __x86_64__
    return "linux64";
#else
    return "linux32";
#endif
#else
    return "unknown";
#endif
}

// 工厂函数
std::unique_ptr<fmu_processor> create_fmu_processor()
{
    return std::unique_ptr<fmu_processor>(new fmu_processor());
}

} // namespace fmi
} // namespace cosmo