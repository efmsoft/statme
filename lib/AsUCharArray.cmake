function(AsUCharArray in_file array_var row_size out_cpp_file out_hpp_file)
    set(input_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${in_file}")
    set(stamp_file "${CMAKE_BINARY_DIR}/${array_var}_stamp.txt")

    configure_file(${input_file_path} ${stamp_file} COPYONLY)

    file(SIZE ${input_file_path} file_size)
    file(READ ${input_file_path} file_content HEX)

    math(EXPR hex_size "${file_size} * 2")
    set(array_content "std::array<unsigned char, ${file_size}> ${array_var} = {\n  ")
    set(line_length 0)
    math(EXPR row_size "${row_size} * 2")

    foreach(index RANGE 0 ${hex_size} 2)
        if(index EQUAL hex_size)
            string(APPEND array_content "\n};")
        else()
            string(SUBSTRING "${file_content}" ${index} 2 hex_char)
            string(APPEND array_content "0x${hex_char}")
            math(EXPR line_length "${line_length} + 2")
            string(APPEND array_content ", ")
            if(line_length EQUAL ${row_size})
                string(APPEND array_content "\n  ")
                set(line_length 0)
            endif()
        endif()
    endforeach()

    set(header_file "${CMAKE_BINARY_DIR}/${array_var}Array.hpp")
    set(source_file "${CMAKE_BINARY_DIR}/${array_var}Array.cpp")

    file(WRITE ${header_file} "#pragma once\n\n#include <array>\n\nextern std::array<unsigned char, ${file_size}> ${array_var};\n")
    file(WRITE ${source_file} "#include \"${array_var}Array.hpp\"\n\n${array_content}\n")

    set(${out_hpp_file} ${header_file} PARENT_SCOPE)
    set(${out_cpp_file} ${source_file} PARENT_SCOPE)
endfunction()
