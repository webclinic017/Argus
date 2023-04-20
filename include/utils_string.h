//
// Created by Nathan Tormaschy on 4/19/23.
//

#ifndef ARGUS_UTILS_STRING_H
#define ARGUS_UTILS_STRING_H


bool case_ins_str_compare(const std::string& str1, const std::string& str2) {
    return strcasecmp(str1.c_str(), str2.c_str()) == 0;
}

size_t case_ins_str_index(const vector<std::string> &columns, string& column){
    auto it = std::find_if(columns.begin(), columns.end(), [&column](const std::string& s) {
        return case_ins_str_compare(s, column);
    });

    if (it != columns.end()) {
        size_t open_index = std::distance(columns.begin(), it);
        return open_index;
    } else {
        throw runtime_error("failed to find column: " + column);
    }
}

tuple<size_t , size_t > parse_headers(const vector<std::string> &columns){
    return std::make_tuple(
            case_ins_str_index(columns, (string &) "open"),
    case_ins_str_index(columns, (string &) "close"));

}

#endif //ARGUS_UTILS_STRING_H
