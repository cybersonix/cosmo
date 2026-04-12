#ifndef COSMO_FMI_FMU_PROCESSOR_HPP
#define COSMO_FMI_FMU_PROCESSOR_HPP

#include "cosmo/fmi/model_description.hpp"
#include "cosmo/util/filesystem.hpp"
#include "cosmo/util/xml_parser.hpp"

#include <string>
#include <memory>
#include <vector>

namespace cosmo {
namespace fmi {

// FMU文件条目类型
enum class fmu_entry_type
{
    file,
    directory
};

// FMU文件条目
struct fmu_entry
{
    std::string name;
    fmu_entry_type type;
    size_t size;
    size_t compressed_size;
    std::vector<unsigned char> data;
};

// FMU处理器类
class fmu_processor
{
public:
    fmu_processor();
    ~fmu_processor();

    // 加载FMU文件
    bool load_fmu(const std::string& fmu_path);

    // 解压FMU到指定目录
    bool extract_to_directory(const std::string& output_dir);

    // 获取FMU条目列表
    std::vector<std::string> get_entry_names() const;

    // 获取指定条目的内容
    bool get_entry_content(const std::string& entry_name, std::vector<unsigned char>& content);

    // 解析modelDescription.xml
    std::unique_ptr<model_description> parse_model_description();

    // 获取FMU的二进制文件路径（用于加载动态库）
    std::string get_binary_path(const std::string& platform = "") const;

    // 获取临时解压目录
    const std::string& get_extract_dir() const { return extract_dir_; }

    // 清理临时文件
    void cleanup();

private:
    // 解压ZIP文件
    bool unzip_fmu(const std::string& fmu_path, const std::string& output_dir);

    // 解析ZIP文件头
    bool parse_zip_header(const std::vector<unsigned char>& data, size_t& pos);

    // 解析ZIP中央目录
    bool parse_central_directory(const std::vector<unsigned char>& data, size_t pos);

    // 解压单个条目
    bool decompress_entry(const fmu_entry& entry, std::vector<unsigned char>& output);

    // 解析modelDescription.xml内容
    std::unique_ptr<model_description> parse_model_description_xml(const std::string& xml_content);

    // 解析FMI 3.0变量（直接Float64/Int32/Boolean/String元素）
    void parse_fmi3_variables(const std::shared_ptr<util::xml_node>& model_variables,
                              model_description* model_desc);

    // 获取当前平台名称
    std::string get_current_platform() const;

private:
    std::string fmu_path_;
    std::string extract_dir_;
    std::vector<fmu_entry> entries_;
    bool loaded_;
};

// 工厂函数
std::unique_ptr<fmu_processor> create_fmu_processor();

} // namespace fmi
} // namespace cosmo

#endif // COSMO_FMI_FMU_PROCESSOR_HPP