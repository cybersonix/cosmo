#include "cosmo/util/xml_parser.hpp"
#include "cosmo/logger/logger.hpp"

#include <algorithm>
#include <cctype>

namespace cosmo {
namespace util {

// xml_node implementation
xml_node::xml_node(const std::string& name)
    : name_(name)
{
}

const std::string& xml_node::get_name() const
{
    return name_;
}

void xml_node::set_name(const std::string& name)
{
    name_ = name;
}

const std::string& xml_node::get_text() const
{
    return text_;
}

void xml_node::set_text(const std::string& text)
{
    text_ = text;
}

bool xml_node::has_attribute(const std::string& name) const
{
    return attributes_.find(name) != attributes_.end();
}

std::string xml_node::get_attribute(const std::string& name, const std::string& default_value) const
{
    auto it = attributes_.find(name);
    if (it != attributes_.end()) {
        return it->second;
    }
    return default_value;
}

void xml_node::set_attribute(const std::string& name, const std::string& value)
{
    attributes_[name] = value;
}

const xml_attributes& xml_node::get_attributes() const
{
    return attributes_;
}

void xml_node::add_child(std::shared_ptr<xml_node> child)
{
    if (child) {
        children_.push_back(child);
    }
}

std::shared_ptr<xml_node> xml_node::get_child(const std::string& name) const
{
    for (const auto& child : children_) {
        if (child->get_name() == name) {
            return child;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<xml_node>> xml_node::get_children(const std::string& name) const
{
    std::vector<std::shared_ptr<xml_node>> result;
    for (const auto& child : children_) {
        if (name.empty() || child->get_name() == name) {
            result.push_back(child);
        }
    }
    return result;
}

size_t xml_node::child_count() const
{
    return children_.size();
}

std::shared_ptr<xml_node> xml_node::find_node(const std::string& name) const
{
    for (const auto& child : children_) {
        if (child->get_name() == name) {
            return child;
        }
        auto found = child->find_node(name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<xml_node>> xml_node::find_all_nodes(const std::string& name) const
{
    std::vector<std::shared_ptr<xml_node>> result;
    for (const auto& child : children_) {
        if (child->get_name() == name) {
            result.push_back(child);
        }
        auto found = child->find_all_nodes(name);
        result.insert(result.end(), found.begin(), found.end());
    }
    return result;
}

std::string xml_node::to_string(int indent) const
{
    std::string result;
    std::string indent_str(indent * 2, ' ');

    result += indent_str + "<" + name_;

    // Add attributes
    for (const auto& attr : attributes_) {
        result += " " + attr.first + "=\"" + encode_xml_entities(attr.second) + "\"";
    }

    if (children_.empty() && text_.empty()) {
        result += "/>\n";
    } else {
        result += ">";
        if (!children_.empty()) {
            result += "\n";
            for (const auto& child : children_) {
                result += child->to_string(indent + 1);
            }
            result += indent_str;
        } else {
            result += encode_xml_entities(text_);
        }
        result += "</" + name_ + ">\n";
    }

    return result;
}

// xml_document implementation
xml_document::xml_document()
{
}

bool xml_document::load_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        error_ = "Failed to open file: " + filename;
        cosmo::log::error(error_);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_string(buffer.str());
}

bool xml_document::load_string(const std::string& content)
{
    error_.clear();
    return parse(content);
}

std::shared_ptr<xml_node> xml_document::get_root() const
{
    return root_;
}

void xml_document::set_root(std::shared_ptr<xml_node> root)
{
    root_ = root;
}

bool xml_document::save_file(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        cosmo::log::error("Failed to open file for writing: " + filename);
        return false;
    }

    file << to_string();
    return file.good();
}

std::string xml_document::to_string() const
{
    std::string result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    if (root_) {
        result += root_->to_string(0);
    }
    return result;
}

const std::string& xml_document::get_error() const
{
    return error_;
}

bool xml_document::parse(const std::string& content)
{
    // 检查空内容
    if (content.empty()) {
        error_ = "Empty XML document";
        return false;
    }

    size_t pos = 0;

    // Skip XML declaration if present
    size_t xml_decl_pos = content.find("<?xml", pos);
    if (xml_decl_pos != std::string::npos) {
        pos = content.find("?>", xml_decl_pos);
        if (pos != std::string::npos) {
            pos += 2;
        } else {
            error_ = "Invalid XML declaration";
            return false;
        }
    }

    // Skip whitespace
    while (pos < content.size() && std::isspace(content[pos])) {
        ++pos;
    }

    if (pos >= content.size()) {
        error_ = "Empty XML document";
        return false;
    }

    // Parse root node - create a temporary parent to hold the root
    auto temp_parent = std::make_shared<xml_node>("temp");
    pos = parse_node(content, pos, temp_parent);

    if (pos == std::string::npos) {
        return false;
    }

    // The actual root is the first (and only) child of temp_parent
    if (temp_parent->child_count() > 0) {
        root_ = temp_parent->get_children()[0];
    } else {
        error_ = "Failed to parse root node";
        return false;
    }

    return true;
}

size_t xml_document::parse_node(const std::string& content, size_t pos, std::shared_ptr<xml_node> parent)
{
    // Find opening tag
    size_t tag_start = content.find('<', pos);
    if (tag_start == std::string::npos) {
        return std::string::npos;
    }

    // Parse tag
    std::string tag_name;
    xml_attributes attributes;
    bool is_closing = false;
    bool is_self_closing = false;

    pos = parse_tag(content, tag_start, tag_name, attributes, is_closing, is_self_closing);
    if (pos == std::string::npos) {
        return std::string::npos;
    }

    if (is_closing) {
        // This is a closing tag, return position after it
        return pos;
    }

    // Create node
    auto node = std::make_shared<xml_node>(tag_name);
    for (const auto& attr : attributes) {
        node->set_attribute(attr.first, attr.second);
    }

    if (is_self_closing) {
        parent->add_child(node);
        return pos;
    }

    // Parse content until closing tag
    size_t content_start = pos;
    size_t content_end = content.find("</" + tag_name + ">", pos);
    if (content_end == std::string::npos) {
        error_ = "Missing closing tag for: " + tag_name;
        return std::string::npos;
    }

    std::string node_content = content.substr(content_start, content_end - content_start);

    // Check if content has child nodes
    if (node_content.find('<') != std::string::npos) {
        // Parse children
        size_t child_pos = 0;
        while (child_pos < node_content.size()) {
            // Skip whitespace
            while (child_pos < node_content.size() && std::isspace(node_content[child_pos])) {
                ++child_pos;
            }

            if (child_pos >= node_content.size()) {
                break;
            }

            if (node_content[child_pos] == '<') {
                child_pos = parse_node(node_content, child_pos, node);
                if (child_pos == std::string::npos) {
                    return std::string::npos;
                }
            } else {
                // Text content
                size_t text_end = node_content.find('<', child_pos);
                if (text_end == std::string::npos) {
                    text_end = node_content.size();
                }
                std::string text = parse_text(node_content, child_pos, text_end);
                node->set_text(text);
                child_pos = text_end;
            }
        }
    } else {
        // Pure text content
        node->set_text(parse_text(node_content, 0, node_content.size()));
    }

    parent->add_child(node);

    // Skip closing tag
    pos = content_end + tag_name.size() + 3; // +3 for </>

    return pos;
}

size_t xml_document::parse_tag(const std::string& content, size_t pos, std::string& tag_name,
                               xml_attributes& attributes, bool& is_closing, bool& is_self_closing)
{
    is_closing = false;
    is_self_closing = false;

    // Skip '<'
    ++pos;

    // Check for closing tag
    if (content[pos] == '/') {
        is_closing = true;
        ++pos;
    }

    // Parse tag name
    size_t name_start = pos;
    while (pos < content.size() && !std::isspace(content[pos]) && 
           content[pos] != '>' && content[pos] != '/') {
        ++pos;
    }
    tag_name = content.substr(name_start, pos - name_start);

    if (is_closing) {
        // Skip to '>'
        pos = content.find('>', pos);
        if (pos == std::string::npos) {
            return std::string::npos;
        }
        return pos + 1;
    }

    // Parse attributes
    pos = parse_attributes(content, pos, attributes);

    // Check for self-closing tag
    if (pos < content.size() && content[pos] == '/') {
        is_self_closing = true;
        ++pos;
    }

    // Skip '>'
    if (pos < content.size() && content[pos] == '>') {
        return pos + 1;
    }

    return std::string::npos;
}

size_t xml_document::parse_attributes(const std::string& content, size_t pos, xml_attributes& attributes)
{
    while (pos < content.size()) {
        // Skip whitespace
        while (pos < content.size() && std::isspace(content[pos])) {
            ++pos;
        }

        if (pos >= content.size() || content[pos] == '>' || content[pos] == '/') {
            break;
        }

        // Parse attribute name
        size_t name_start = pos;
        while (pos < content.size() && content[pos] != '=' && !std::isspace(content[pos]) &&
               content[pos] != '>' && content[pos] != '/') {
            ++pos;
        }
        std::string attr_name = content.substr(name_start, pos - name_start);

        // Skip whitespace and '='
        while (pos < content.size() && std::isspace(content[pos])) {
            ++pos;
        }
        if (pos < content.size() && content[pos] == '=') {
            ++pos;
        }

        // Skip whitespace
        while (pos < content.size() && std::isspace(content[pos])) {
            ++pos;
        }

        // Parse attribute value
        if (pos < content.size() && (content[pos] == '"' || content[pos] == '\'')) {
            char quote = content[pos];
            ++pos;
            size_t value_start = pos;
            pos = content.find(quote, pos);
            if (pos == std::string::npos) {
                return std::string::npos;
            }
            std::string attr_value = content.substr(value_start, pos - value_start);
            attributes[attr_name] = decode_entities(attr_value);
            ++pos;
        }
    }

    return pos;
}

std::string xml_document::parse_text(const std::string& content, size_t start, size_t end)
{
    std::string text = content.substr(start, end - start);
    return decode_entities(trim(text));
}

std::string xml_document::trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string xml_document::decode_entities(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '&') {
            size_t end = str.find(';', i);
            if (end != std::string::npos) {
                std::string entity = str.substr(i + 1, end - i - 1);
                if (entity == "lt") {
                    result += '<';
                } else if (entity == "gt") {
                    result += '>';
                } else if (entity == "amp") {
                    result += '&';
                } else if (entity == "quot") {
                    result += '"';
                } else if (entity == "apos") {
                    result += '\'';
                } else {
                    result += str[i];
                }
                i = end;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string xml_document::encode_entities(const std::string& str)
{
    std::string result;
    result.reserve(str.size() * 2);

    for (char c : str) {
        switch (c) {
            case '<':
                result += "<";
                break;
            case '>':
                result += ">";
                break;
            case '&':
                result += "&";
                break;
            case '"':
                result += "\"";
                break;
            case '\'':
                result += "'";
                break;
            default:
                result += c;
                break;
        }
    }

    return result;
}

// Factory function
std::unique_ptr<xml_document> create_xml_document()
{
    return std::unique_ptr<xml_document>(new xml_document());
}

// Free function for XML entity encoding
std::string encode_xml_entities(const std::string& str)
{
    std::string result;
    result.reserve(str.size() * 1.1);

    for (char c : str) {
        switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c;        break;
        }
    }
    return result;
}

} // namespace util
} // namespace cosmo