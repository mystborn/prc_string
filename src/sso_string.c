/*
    MIT License

    Copyright (c) 2019 Precisamento LLC
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <sso_string.h>

#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#if defined(SSO_STRING_SINGLE_THREAD)

#define SSO_THREAD_LOCAL

#elif defined(_MSC_VER)

#define SSO_THREAD_LOCAL __declspec(thread)

#elif defined(__GNUC__) || defined(__clang__)

#define SSO_THREAD_LOCAL __thread

#elif __STDC_VERSION__ >= 201112L
#ifndef __STDC_NO_THREADS__
#ifdef thread_local

#include <threads.h>

#define SSO_THREAD_LOCAL thread_local

#endif
#endif
#endif

// These are used to check the first character in UTF-8 sequence to determine
// the length of the codepoint in bytes.

#define U8_SINGLE 0x7F
#define U8_DOUBLE 0xE0
#define U8_TRIPLE 0xF0
#define U8_QUAD   0xF8 

SSO_STRING_EXPORT bool string_init(String* str, const char* cstr) {
    SSO_STRING_ASSERT_ARG(str);

    // Allow users to pass NULL as the cstr parameter which
    // will just create an empty string.
    if(cstr == NULL)
        cstr = "";

    size_t len = strlen(cstr);
    if (len <= SSO_STRING_MIN_CAP) {
        memcpy(str->s.data, cstr, len);
        str->s.data[len] = 0;

        sso_string_short_set_size(str, len);
    } else {
        size_t cap = sso_string_next_cap(0, len);
        str->l.data = sso_string_malloc(cap + 1);
        if(!str->l.data)
            return false;
        memcpy(str->l.data, cstr, len);
        str->l.data[len] = 0;
        str->l.size = len;
        sso_string_long_set_cap(str, cap);
    }

    return true;
}

SSO_STRING_EXPORT bool string_init_size(String* str, const char* cstr, size_t len) {
    // Allow users to pass NULL as the cstr parameter which
    // will just create an empty string.
    if(cstr == NULL)
        cstr = "";

    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_BOUNDS(len <= strlen(cstr));

    if (len <= SSO_STRING_MIN_CAP) {
        memcpy(str->s.data, cstr, len);
        str->s.data[len] = 0;

        sso_string_short_set_size(str, len);
    } else {
        size_t cap = sso_string_next_cap(0, len);
        str->l.data = sso_string_malloc(cap + 1);
        if(!str->l.data)
            return false;
        memcpy(str->l.data, cstr, len);
        str->l.data[len] = 0;
        str->l.size = len;
        sso_string_long_set_cap(str, cap);
    }

    return true;
}

SSO_STRING_EXPORT size_t string_u8_codepoints(const String* str) {
    SSO_STRING_ASSERT_ARG(str);

    const unsigned char* data = (unsigned char*)string_data(str);
    size_t count = 0;
    while(*data) {
        count++;

        if(*data  <= U8_SINGLE)
            data += 1;
        else if(*data < U8_DOUBLE)
            data += 2;
        else if(*data < U8_TRIPLE)
            data += 3;
        else
            data += 4;
    }

    return count;
}

SSO_STRING_EXPORT Char32 string_u8_get(const String* str, size_t index) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    if(index >= size)
        return '\0';

    const unsigned char* data = (unsigned char*)string_data(str) + index;

    // TODO: Test for control characters (0x80 - 0xBF)

    if(*data <= U8_SINGLE) {
        return *data;
    } else if(*data < U8_DOUBLE) {
        return (((*data) & ~U8_DOUBLE) << 6) | ((*(data + 1)) & 0x3f);
    } else if(*data < U8_TRIPLE) {
        return (((*data) & ~U8_TRIPLE) << 12) | (((*(data + 1)) & 0x3f) << 6) | ((*(data + 2)) & 0x3f);
    } else {
        return (((*data) & ~U8_QUAD) << 18) | (((*(data + 1)) & 0x3f) << 12) | (((*(data + 2)) & 0x3f) << 6) | ((*(data + 3)) & 0x3f);
    }
}

SSO_STRING_EXPORT Char32 string_u8_get_with_size(const String* str, size_t index, int* out_size) {
    SSO_STRING_ASSERT_ARG(str);

    if(!out_size)
        return string_u8_get(str, index);

    size_t size = string_size(str);
    if(index >= size) {
        *out_size = 0;
        return '\0';
    }

    const unsigned char* data = (unsigned char*)string_data(str) + index;

    if(*data  <= U8_SINGLE) {
        *out_size = 1;
        return *data;
    } else if(*data < U8_DOUBLE) {
        *out_size = 2;
        return (((*data) & ~U8_DOUBLE) << 6) | ((*(data + 1)) & 0x3f);
    } else if(*data < U8_TRIPLE) {
        *out_size = 3;
        return (((*data) & ~U8_TRIPLE) << 12) | (((*(data + 1)) & 0x3f) << 6) | ((*(data + 2)) & 0x3f);
    } else {
        *out_size = 4;
        return (((*data) & ~U8_QUAD) << 18) | (((*(data + 1)) & 0x3f) << 12) | (((*(data + 2)) & 0x3f) << 6) | ((*(data + 3)) & 0x3f);
    }
}

SSO_STRING_EXPORT int string_u8_codepoint_size(const String* str, size_t index) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    if(index >= size)
        return 0;

    const char* data = string_data(str) + index;

    if((unsigned char)*data  <= U8_SINGLE)
        return 1;
    else if((unsigned char)*data < U8_DOUBLE)
        return 2;
    else if((unsigned char)*data < U8_TRIPLE)
        return 3;
    else
        return 4;
}

/** 
    Resizes the string smaller or larger to make room to replace a UTF-8 codepoint.
*/
static bool sso_string_u8_assure_codepoint_space(
    String* str, 
    size_t index, 
    int old_codepoint, 
    int new_codepoint) 
{
    if(new_codepoint == old_codepoint)
        return true;

    int offset = new_codepoint - old_codepoint;
    size_t size = string_size(str);

    if(offset > 0) {
        if(!string_reserve(str, size + offset))
            return false;
    }

    char* data = string_cstr(str);
    char* ptr = data;

    memmove(data + index + new_codepoint, data + index + old_codepoint, size - index - old_codepoint);
    ptr = data;
    sso_string_set_size(str, size + offset);
    data[size + offset] = '\0';

    return true;
}

/*
    Convenience function to get the number of bytes needed to represent
    a UTF-8 codepoint as a char*.
*/
static int sso_string_u8_codepoint_size(Char32 value) {
    if(value < 0x80)
        return 1;
    else if (value < 0x800)
        return 2;
    else if(value < 0x10000)
        return 3;
    else {
        return 4;
    }
}

/*
    Convenience function to set a codepoint in the underlying c-string of a String.
*/
static void sso_string_u8_set_underlying(char* data, Char32 value, size_t index, int size) {
    switch(size) {
        case 1:
            data[index] = (char)value;
            break;
        case 2:
            data[index]     = 0xC0 | ((value >> 6) & 0x1F);
            data[index + 1] = 0x80 | (value & 0x3f);
            break;
        case 3:
            data[index]     = 0xE0 | ((value >> 12) & 0xF);
            data[index + 1] = 0x80 | ((value >> 6)  & 0x3F);
            data[index + 2] = 0x80 | (value & 0x3F);
            break;
        case 4:
            data[index]     = 0xF0 | ((value >> 18) & 0x7);
            data[index + 1] = 0x80 | ((value >> 12) & 0x3F);
            data[index + 2] = 0x80 | ((value >> 6)  & 0x3F);
            data[index + 3] = 0x80 | (value & 0x3F);
            break;
    }
}

SSO_STRING_EXPORT bool string_u8_set(String* str, size_t index, Char32 value) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);

    assert(index <= string_size(str));

    if(index == size)
        return string_u8_push(str, value);

    int old_codepoint = string_u8_codepoint_size(str, index);
    int new_codepoint = sso_string_u8_codepoint_size(value);
    if(!sso_string_u8_assure_codepoint_space(str, index, old_codepoint, new_codepoint))
        return false;

    char* data = string_cstr(str);
    sso_string_u8_set_underlying(data, value, index, new_codepoint);

    return true;
}

SSO_STRING_EXPORT void string_upper(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);

    char* data = string_cstr(str);

    for(size_t i = 0; i < size; i++) {
        data[i] = toupper(data[i]);
    }
}

SSO_STRING_EXPORT void string_u8_upper(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    size_t index = 0;
    int letter_size = 1;
    char* data = string_cstr(str);

    while(index < size) {
        Char32 letter = string_u8_get_with_size(str, index, &letter_size);
        letter = towupper(letter);
        sso_string_u8_set_underlying(data, letter, index, letter_size);
        index += letter_size;
    }
}

SSO_STRING_EXPORT void string_lower(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);

    char* data = string_cstr(str);

    for(size_t i = 0; i < size; i++) {
        data[i] = tolower(data[i]);
    }
}

SSO_STRING_EXPORT void string_u8_lower(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    size_t index = 0;
    int letter_size = 1;
    char* data = string_cstr(str);

    while(index < size) {
        Char32 letter = string_u8_get_with_size(str, index, &letter_size);
        letter = towlower(letter);
        sso_string_u8_set_underlying(data, letter, index, letter_size);
        index += letter_size;
    }
}

SSO_STRING_EXPORT bool string_u8_is_null_or_whitespace(const String* str) {
    if(string_is_null_or_empty(str))
        return true;

    size_t size = string_size(str);
    size_t i = 0;

    while(i < size) {
        int cp;
        Char32 next = string_u8_get_with_size(str, i, &cp);

        // These are the UTF-8 whitespace codepoints as defined by Wikipedia.
        // Includes the related characters that aren't necessarily traditional whitespace
        // characters (such as the zero width space '\u200B').
        switch(next) {
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 32:
            case 133:
            case 160:
            case 5760:
            case 6158:
            case 8192:
            case 8193:
            case 8194:
            case 8195:
            case 8196:
            case 8197:
            case 8198:
            case 8199:
            case 8200:
            case 8201:
            case 8202:
            case 8203:
            case 8204:
            case 8205:
            case 8232:
            case 8233:
            case 8287:
            case 8288:
            case 12288:
            case 65279:
                break;
            default:
                return false;
        }

        i += cp;
    }

    return true;
}

SSO_STRING_EXPORT bool sso_string_long_reserve(String* str, size_t reserve) {
    SSO_STRING_ASSERT_ARG(str);

    size_t current = sso_string_long_cap(str);
    if(reserve <= current)
        return true;

    reserve = sso_string_next_cap(current, reserve);
    void* buffer = sso_string_realloc(str->l.data, reserve + 1);
    if(!buffer)
        return false;

    str->l.data = buffer;
    str->l.data[reserve] = 0;

    sso_string_long_set_cap(str, reserve);
    return true;
}

SSO_STRING_EXPORT int sso_string_short_reserve(String* str, size_t reserve) {
    SSO_STRING_ASSERT_ARG(str);

    if(reserve <= SSO_STRING_MIN_CAP)
        return SSO_STRING_SHORT_RESERVE_SUCCEED;

    reserve = sso_string_next_cap(SSO_STRING_MIN_CAP, reserve);
    char* data = sso_string_malloc(sizeof(char) * (reserve + 1));
    if(!data)
        return SSO_STRING_SHORT_RESERVE_FAIL;

    memmove(data, str->s.data, sso_string_short_size(str));
    data[reserve] = 0;

    str->l.size = sso_string_short_size(str);
    str->l.data = data;

    sso_string_long_set_cap(str, reserve);

    return SSO_STRING_SHORT_RESERVE_RESIZE;
}

SSO_STRING_EXPORT void string_shrink_to_fit(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    if(!sso_string_is_long(str))
        return;

    size_t s = sso_string_long_size(str);

    if(s == sso_string_long_cap(str))
        return;

    if(s <= SSO_STRING_MIN_CAP) {
        memmove(str->s.data, str->l.data, s);
        str->s.data[s] = 0;
        // This will clear the long flag.
        sso_string_short_set_size(str, s);
    } else {
        str->l.data = sso_string_realloc(str->l.data, s + 1);
        str->l.data[s] = 0;
        sso_string_long_set_cap(str, s);
    }
}

SSO_STRING_EXPORT bool sso_string_insert_impl(String* str, const char* value, size_t index, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    size_t current_size = string_size(str);

    SSO_STRING_ASSERT_BOUNDS(current_size + length < STRING_MAX);

    char* start_data = string_cstr(str);
    size_t offset = value - start_data;

    if(!string_reserve(str, current_size + length))
        return false;
    char* data = string_cstr(str);

    // Make room for the inserted value.
    // The check is there since this operation is not needed if
    // the value is being appended to the end of the string.
    if(index != current_size)
        memmove(data + index + length, data + index, current_size - index);

    // This section is triggered when value is a part of str.
    // Special consideration needs to be taken when the internal
    // c-string is moved as a result of the string_reserve operation.
    if(offset < current_size) {
        if(offset + length <= index) {
            // The section of the same string is before the insert point.
            // In this case we can just adjust the value pointer to the new location.
            value = data + offset;
            memmove(data + index, value, length);
        } else if(offset >= index) {
            // The section of the same string is after the insert point.
            // In this case we can adjust the the value pointer to the realloc-ed
            // location plus the length of the blank area created to make space
            // for the insert.
            value = data + offset + length;
            memmove(data + index, value, length);
        } else {
            // The insert point is in the middle of the string section.
            // In string section will have been split to make room for the
            // insert. In this case, just move the split sections into the
            // insert area separately.
            value = data + offset;
            size_t begin_length = index - offset;
            memmove(data + index, value, begin_length);
            value = data + offset + length + begin_length;
            memmove(data + index + begin_length, value, length - begin_length);
        }
    } else {
        memmove(data + index, value, length);
    }

    data[current_size + length] = 0;
    sso_string_set_size(str, current_size + length);
    return true;
}

SSO_STRING_EXPORT void string_erase(String* str, size_t index, size_t count) {
    SSO_STRING_ASSERT_ARG(str);
    if(count == 0)
        return;

    size_t current_size = string_size(str);
    assert(index + count <= current_size);
    char* data = string_cstr(str);
    memmove(data + index, data + index + count, current_size - index - count);
    current_size -= count;
    if(sso_string_is_long(str)) {
        sso_string_long_set_size(str, current_size);
        str->l.data[current_size] = 0;
    } else {
        sso_string_short_set_size(str, current_size);
        str->s.data[current_size] = 0;
    }
}

SSO_STRING_EXPORT bool string_push(String* str, char value) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size;
    char* data;

    if(sso_string_is_long(str)) {
        size = sso_string_long_size(str) + 1;
        if(!sso_string_long_reserve(str, size))
            return false;
        sso_string_long_set_size(str, size);
        data = str->l.data;
    } else {
        size = sso_string_short_size(str) + 1;
        switch(sso_string_short_reserve(str, size)) {
            case SSO_STRING_SHORT_RESERVE_RESIZE:
                sso_string_long_set_size(str, size);
                data = str->l.data;
                break;
            case SSO_STRING_SHORT_RESERVE_SUCCEED:
                sso_string_short_set_size(str, size);
                data = str->s.data;
                break;
            case SSO_STRING_SHORT_RESERVE_FAIL:
                return false;
        }
    }
    data[size-1] = value;
    data[size] = 0;
    return true;
}

SSO_STRING_EXPORT bool string_u8_push(String* str, Char32 value) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    char* data;
    
    if(value < 0x80) {
        if(!string_reserve(str, size + 1))
            return false;
        data = string_cstr(str);
        data[size++] = (char)value;
    } else if(value < 0x800) {
        if(!string_reserve(str, size + 2))
            return false;

        data = string_cstr(str);
        data[size++] = 0xC0 | ((value >> 6) & 0x1F);
        data[size++] = 0x80 | (value & 0x3f);

    } else if(value < 0x10000) {
        if(!string_reserve(str, size + 3))
            return false;

        data = string_cstr(str);

        data[size++] = 0xE0 | ((value >> 12) & 0xF);
        data[size++] = 0x80 | ((value >> 6)  & 0x3F);
        data[size++] = 0x80 | (value & 0x3F);
    } else {
        assert(value <= 0x10FFFF);
        if(!string_reserve(str, size + 4))
            return false;

        data = string_cstr(str);

        data[size++] = 0xF0 | ((value >> 18) & 0x7);
        data[size++] = 0x80 | ((value >> 12) & 0x3F);
        data[size++] = 0x80 | ((value >> 6)  & 0x3F);
        data[size++] = 0x80 | (value & 0x3F);
    }

    sso_string_set_size(str, size);
    data[size] = '\0';
    return true;
}

SSO_STRING_EXPORT char string_pop(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size;
    if(sso_string_is_long(str)) {
        size = sso_string_long_size(str) - 1;
        if(size == -1)
            return 0;
        sso_string_long_set_size(str, size);
    } else {
        size = sso_string_short_size(str) - 1;
        if(size == -1)
            return 0;
        sso_string_short_set_size(str, size);
    }
    char* data = string_cstr(str);
    char result = data[size];
    data[size] = 0;
    return result;
}

SSO_STRING_EXPORT Char32 string_u8_pop(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    if(size == 0)
        return 0;

    char* data = string_cstr(str);
    int shift = 0;
    Char32 result = 0;

    unsigned char part = (unsigned char)data[--size];
    while((part & 0xC0) == 0x80) {
        unsigned int stripped_part = (~0x80 & part);
        unsigned int shifted = (stripped_part << (6 * shift++));
        result |= shifted;
        part = (unsigned char)data[--size];
    }

    if(part >= 0xF0)
        result |= (part & 0x07) << 18;
    else if(part >= 0xE0)
        result |= (part & 0x0F) << 12;
    else if(part >= 0xC0)
        result |= (part & 0x1F) << 6;
    else
        result |= part;

    sso_string_set_size(str, size);
    data[size] = 0;

    return result;
}

SSO_STRING_EXPORT bool sso_string_append_impl(String* str, const char* value, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    size_t size = string_size(str);
    const char* start_data = string_data(str);

    // Allow the offset to wrap so the conditional doesn't have to check
    // negative numbers.
    size_t offset = value - start_data;

    if(!string_reserve(str, size + length))
        return false;
    char* data = string_cstr(str);

    // Allow string to append itself without error on resize.
    if(data != start_data && offset < size)
        value = data + offset;

    memmove(data + size, value, length);
    data[size + length] = 0;
    sso_string_set_size(str, size + length);
    return true;
}

SSO_STRING_EXPORT void sso_string_trim_start_impl(String* str, const char* value, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    const char* data = string_data(str);
    size_t count = 0;
    size_t size = string_size(str);

    for(; count < size; count += length) {
        if(strncmp(data + count, value, length) != 0)
            break;
    }

    if(count != 0) {
        string_erase(str, 0, count);
    }
}

SSO_STRING_EXPORT void sso_string_trim_end_impl(String* str, const char* value, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    size_t size = string_size(str);
    char* data = string_cstr(str);
    size_t count = size;

    for(; count > 0; count -= length) {
        if(strncmp(data + count - length, value, length) != 0)
            break;
    }

    if(count != size) {
        data[count] = '\0';
        sso_string_set_size(str, count);
    }
}

SSO_STRING_EXPORT void string_trim_any_start_string(String* str, String* values, size_t value_count) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(values);

    const char* data = string_data(str);
    size_t count = 0;
    size_t size = string_size(str);

    size_t found_length = 0;
    
    do {
        found_length = 0;
        for(size_t i = 0; i < value_count; i++) {
            size_t look_size = string_size(values + i);

            // Only perform the comparison if the substring is longer than the last found
            // string. This is used in cases where later strings in the array start the same
            // as a previous option.
            // For example, imagine this input string "hello world" and this trim array:
            // ["he", "h", "abc"].
            // In this case we don't want to perform the string comparison for "h" since
            // even if it matches it'd be shorter than the already matched "he". However
            // we would want to check "abc" since if it did match it would result in a longer
            // offset.
            // Also there's no need to perform the comparison of the substring size is longer
            // than the size of the string.
            if(look_size > found_length && look_size <= size - count) {
                if(strncmp(data + count, string_data(values + i), look_size) == 0) {
                    found_length = look_size;
                }
            }
        }

        count += found_length;
    }
    while(count < size && found_length != 0);

    if(count != 0) {
        string_erase(str, 0, count);
    }
}

SSO_STRING_EXPORT void string_trim_any_start_string_refs(String* str, String** values, size_t value_count) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(values);

    const char* data = string_data(str);
    size_t count = 0;
    size_t size = string_size(str);

    size_t found_length = 0;
    
    do {
        found_length = 0;
        for(size_t i = 0; i < value_count; i++) {
            size_t look_size = string_size(values[i]);

            // See the comment in string_trim_any_start_string.
            if(look_size > found_length && look_size <= size - count) {
                if(strncmp(data + count, string_data(values[i]), look_size) == 0) {
                    found_length = look_size;
                }
            }
        }

        count += found_length;
    }
    while(count < size && found_length != 0);

    if(count != 0) {
        string_erase(str, 0, count);
    }
}

SSO_STRING_EXPORT void string_trim_any_start_cstr(String* str, char** values, size_t value_count) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(values);

    const char* data = string_data(str);
    size_t count = 0;
    size_t size = string_size(str);

    size_t found_length = 0;
    
    do {
        found_length = 0;
        for(size_t i = 0; i < value_count; i++) {
            size_t look_size = strlen(values[i]);

            // See the comment in string_trim_any_start_string.
            if(look_size > found_length && look_size <= size - count) {
                if(strncmp(data + count, values[i], look_size) == 0) {
                    found_length = look_size;
                }
            }
        }

        count += found_length;
    }
    while(count < size && found_length != 0);

    if(count != 0) {
        string_erase(str, 0, count);
    }
}

SSO_STRING_EXPORT void string_trim_any_end_string(String* str, String* values, size_t value_count) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(values);

    size_t size = string_size(str);
    char* data = string_cstr(str);
    size_t count = size;

    size_t found_length = 0;

    do {
        found_length = 0;
        for(size_t i = 0; i < value_count; i++) {
            size_t look_size = string_size(values + i);

            // Only perform the comparison if the substring is longer than the last found
            // string. This is used in cases where later strings in the array start the same
            // as a previous option.
            // For example, imagine this input string "hello world" and this trim array:
            // ["ld", "d", "abc"].
            // In this case we don't want to perform the string comparison for "d" since
            // even if it matches it'd be shorter than the already matched "ld". However
            // we would want to check "abc" since if it did match it would result in a longer
            // offset.
            // Also there's no need to perform the comparison of the substring size is longer
            // than the size of the string.
            if(look_size > found_length && look_size <= count) {
                if(strncmp(data + count - look_size, string_data(values + i), look_size) == 0) {
                    found_length = look_size;
                }
            }
        }

        count -= found_length;
    }
    while(count > 0 && found_length != 0);

    if(count != size) {
        data[count] = '\0';
        sso_string_set_size(str, count);
    }
}

SSO_STRING_EXPORT void string_trim_any_end_string_refs(String* str, String** values, size_t value_count) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(values);

    size_t size = string_size(str);
    char* data = string_cstr(str);
    size_t count = size;

    size_t found_length = 0;

    do {
        found_length = 0;
        for(size_t i = 0; i < value_count; i++) {
            size_t look_size = string_size(values[i]);

            // See the comment in string_trim_any_end_string.
            if(look_size > found_length && look_size <= count) {
                if(strncmp(data + count - look_size, string_data(values[i]), look_size) == 0) {
                    found_length = look_size;
                }
            }
        }

        count -= found_length;
    }
    while(count > 0 && found_length != 0);

    if(count != size) {
        data[count] = '\0';
        sso_string_set_size(str, count);
    }
}

SSO_STRING_EXPORT void string_trim_any_end_cstr(String* str, char** values, size_t value_count) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(values);

    size_t size = string_size(str);
    char* data = string_cstr(str);
    size_t count = size;

    size_t found_length = 0;

    do {
        found_length = 0;
        for(size_t i = 0; i < value_count; i++) {
            size_t look_size = strlen(values[i]);

            // See the comment in string_trim_any_end_string.
            if(look_size > found_length && look_size <= count) {
                if(strncmp(data + count - look_size, values[i], look_size) == 0) {
                    found_length = look_size;
                }
            }
        }

        count -= found_length;
    }
    while(count > 0 && found_length != 0);

    if(count != size) {
        data[count] = '\0';
        sso_string_set_size(str, count);
    }
}

SSO_STRING_EXPORT bool string_pad_left(String* str, char value, size_t width) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    if(size >= width)
        return true;

    if(!string_reserve(str, width))
        return false;

    char* data = string_cstr(str);
    size_t offset = width - size;

    memmove(data + offset, data, size);
    for(size_t i = 0; i < offset; i++)
        data[i] = value;

    data[width] = '\0';
    sso_string_set_size(str, width);

    return true;
}

SSO_STRING_EXPORT bool string_pad_right(String* str, char value, size_t width) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    if(size >= width)
        return true;

    if(!string_reserve(str, width))
        return false;

    char* data = string_cstr(str);

    for(size_t i = size; i < width; i++)
        data[i] = value;

    data[width] = '\0';
    sso_string_set_size(str, width);

    return true;
}

SSO_STRING_EXPORT bool string_u8_pad_left(String* str, Char32 value, size_t width) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_BOUNDS(value <= 0x10FFFF);

    size_t codepoints = 0;
    size_t size = string_size(str);

    for(size_t i = 0; i < size; i += string_u8_codepoint_size(str, i)) {
        // Early exit if the string has more codepoints than the pad width.
        if(++codepoints >= width)
            return true;
    }

    int codepoint_size = sso_string_u8_codepoint_size(value);
    char* data = string_cstr(str);

    size_t offset = width - codepoints;
    size_t actual_size = size + offset * codepoint_size;
    string_reserve(str, actual_size);

    memmove(data + offset * codepoint_size, data, string_size(str));
    for(size_t i = 0; i < offset; i++)
        sso_string_u8_set_underlying(data, value, i * codepoint_size, codepoint_size);

    data[actual_size] = '\0';
    sso_string_set_size(str, actual_size);

    return true;
}

SSO_STRING_EXPORT bool string_u8_pad_right(String* str, Char32 value, size_t width) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_BOUNDS(value <= 0x10FFFF);

    size_t codepoints = 0;
    size_t size = string_size(str);

    for(size_t i = 0; i < size; i += string_u8_codepoint_size(str, i)) {
        // Early exit if the string has more codepoints than the pad width.
        if(++codepoints >= width)
            return true;
    }

    int codepoint_size = sso_string_u8_codepoint_size(value);
    char* data = string_cstr(str);

    size_t offset = width - codepoints;
    size_t actual_size = size + offset * codepoint_size;
    string_reserve(str, actual_size);

    for(size_t i = 0; i < offset; i++)
        sso_string_u8_set_underlying(data, value, size + i * codepoint_size, codepoint_size);

    data[actual_size] = '\0';
    sso_string_set_size(str, actual_size);

    return true;
}

SSO_STRING_EXPORT bool sso_string_replace_impl(String* str, size_t pos, size_t count, const char* value, size_t length) {
    if(length == 0)
        string_erase(str, pos, count);

    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    size_t size = string_size(str);

    SSO_STRING_ASSERT_BOUNDS(pos + count <= size);

    char* data;

    
    // There are three replace cases:
    // * The length of the replace section is the same as the substring.
    // * The length of the replace section is longer than the substring.
    // * The length of the replace section is shorter than the substring.
    //
    // The three following if blocks are implemented respectively.
    //
    // The first two cases don't need any special consideration for replacing a section
    // of the same string.
    // The final case can potentially move the data pointer from a realloc operation.
    // It also need to handle the case of replacing an overlapping section in a special
    // way since the start and end of the string will be on different sides of the blank
    // area created to make room for the string.
    if(count == length) {
        data = (char*)string_cstr(str);
        memmove(data + pos, value, length);
    } else if(count > length) {
        data = (char*)string_cstr(str);
        size_t offset = pos + count;
        // Make sure to move the substring into the replace position first
        // to make sure replacing an overlapping part of the same string
        // works properly.
        memmove(data + pos, value, length);
        memmove(data + pos + length, data + offset, size - offset);
        size_t end = pos + length + (size - offset);
        data[end] = 0;
        if(sso_string_is_long(str))
            sso_string_long_set_size(str, end);
        else
            sso_string_short_set_size(str, end);
    } else {
        size_t move_offset = length - count;

        data = string_cstr(str);
        size_t offset = value - data;

        if(!string_reserve(str, size + move_offset))
            return false;

        // Set the data pointer again in case the internal string
        // was realloced.
        data = string_cstr(str);

        // Move the section after the replace part to the right to make
        // space for the substring.
        memmove(data + pos + length, data + pos + count, size - (pos + count));

        if(offset < size) {
            if(offset + length <= pos) {
                // Update the value pointer in case of realloc.
                value = data + offset;
                memmove(data + pos, value, length);
            } else if (offset >= pos + length) {
                // Update the value pointer in case of realloc.
                value = data + offset + length - count;
                memmove(data + pos, value, length);
            } else {
                // The replace string and the substring are overlapping.
                // Copy everything from the start until then end of the replace 
                // section into the blank section. Then copy everything past the 
                // end of the replace string into the blank section.

                ptrdiff_t back_length = (offset + length) - (pos + count);

                // back_length may be negative, so don't perform the move in that instance.
                //
                // The math for figuring out the length of the front end is different if
                // the end of the substring is before the end of the replace section.
                if (back_length > 0) {
                    ptrdiff_t front_length = pos + count - offset;
                    memmove(data + pos, data + offset, pos + count - offset);
                    memmove(data + pos + front_length, data + pos + length, back_length);
                } else {
                    memmove(data + pos, data + offset, length);
                }
            }
        } else {
            memmove(data + pos, value, length);
        }

        data[size+move_offset] = 0;
        if(sso_string_is_long(str))
            sso_string_long_set_size(str, size + move_offset);
        else
            sso_string_short_set_size(str, size + move_offset);
    }

    return true;
}

SSO_STRING_EXPORT bool string_resize(String* str, size_t count, char ch) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_BOUNDS(count < STRING_MAX);
    
    if(!string_reserve(str, count))
        return false;

    char* data = string_cstr(str);

    size_t size;

    if(sso_string_is_long(str)) {
        size = sso_string_long_size(str);
        sso_string_long_set_size(str, count);
    } else {
        size = sso_string_short_size(str);
        sso_string_short_set_size(str, count);
    }
    
    for(size_t i = size; i < count; i++)
        data[i] = ch;

    data[count] = 0;
    return true;
}

SSO_STRING_EXPORT size_t sso_string_find_impl(const String* str, size_t pos, const char* value, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    if(pos + length > string_size(str))
        return SIZE_MAX;

    const char* data = string_data(str);
    char* result = strstr(data + pos, value);
    if(result == NULL)
        return SIZE_MAX;
    return result - data;
}

SSO_STRING_EXPORT size_t sso_string_find_part_impl(const String* str, size_t pos, const char* value, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    // If the length of the find string is bigger than the search string,
    // early exit.
    size_t size = string_size(str);
    if(pos + length > size)
        return SIZE_MAX;

    size -= length;

    const char* data = string_data(str);
    for(size_t i = pos; i < size; i++) {
        if(strncmp(data + i, value, length) == 0)
            return i;
    }

    return SIZE_MAX;
}

SSO_STRING_EXPORT size_t sso_string_rfind_impl(const String* str, size_t pos, const char* value, size_t length) {
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(value);

    // If the length of the find string is bigger than the search string,
    // early exit.
    size_t size = string_size(str);

    if(pos > size || length > size)
        return SIZE_MAX;

    if(pos < length)
        pos = length;

    // Turn the negative offset into a normal offset.
    pos = size - pos;

    const char* data = string_data(str);

    do {
        if (strncmp(data + pos, value, length) == 0)
            return pos;
    } 
    while (pos-- != 0);

    return SIZE_MAX;
}

// Todo: Attempt to use intrinsic bswap

static inline void string_reverse_bytes_impl(char* start, char* end) {
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
}

SSO_STRING_EXPORT void string_reverse_bytes(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    char* start = string_cstr(str);
    char* end = start + size - 1;

    string_reverse_bytes_impl(start, end);
}

SSO_STRING_EXPORT void string_u8_reverse_codepoints(String* str) {
    SSO_STRING_ASSERT_ARG(str);

    size_t size = string_size(str);
    char* data;
    char* start = data = string_cstr(str);
    char* end = start + size - 1;

    // Reverse string bytewise
    string_reverse_bytes_impl(start, end);

    start = NULL;

    // Todo: Speed this up
    // Fix up utf8 characters.
    for (size_t i = 0; i < size; i++) {
        switch((*(data + i)) & 0xC0) {
            case 0xC0:
                string_reverse_bytes_impl(start, data + i);
                start = NULL;
                break;
            case 0x80:
                if(!start)
                    start = data + i;
                break;
        }
    }
}

SSO_STRING_EXPORT bool sso_string_join_impl(
    String* str,
    const char* separator,
    size_t separator_length,
    const String* values,
    size_t value_count)
{
    if(value_count == 0)
        return true;

    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(separator);
    SSO_STRING_ASSERT_ARG(values);

    // Put the joined values in a temporary variable to avoid
    // altering the result string if the operation fails.

    // Append the temp string to the result (str) at the end
    // to finalize the operation.
    bool result = false;
    String temp = string_create("");

    if(!string_append_string(&temp, values))
        goto cleanup;

    for(size_t i = 1; i < value_count; i++) {
        if(!string_append_cstr_part(&temp, separator, 0, separator_length))
            goto cleanup;

        if(!string_append_string(&temp, values + i))
            goto cleanup;
    }

    if(!string_append_string(str, &temp))
        goto cleanup;

    result = true;

    cleanup:
        string_free_resources(&temp);

        return result;
}

SSO_STRING_EXPORT bool sso_string_join_refs_impl(
    String* str,
    const char* separator,
    size_t separator_length,
    const String** values,
    size_t value_count)
{
    if(value_count == 0)
        return true;

    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(separator);
    SSO_STRING_ASSERT_ARG(values);

    // Put the joined values in a temporary variable to avoid
    // altering the result string if the operation fails.

    // Append the temp string to the result (str) at the end
    // to finalize the operation.
    bool result = false;
    String temp = string_create("");

    if(!string_append_string(&temp, values[0]))
        goto cleanup;

    for(size_t i = 1; i < value_count; i++) {
        if(!string_append_cstr_part(&temp, separator, 0, separator_length))
            goto cleanup;

        if(!string_append_string(&temp, values[i]))
            goto cleanup;
    }

    if(!string_append_string(str, &temp))
        goto cleanup;

    result = true;

    cleanup:
        string_free_resources(&temp);

        return result;
}

SSO_STRING_EXPORT String* sso_string_split_impl(
    const String* str,
    const char* separator,
    size_t separator_length,
    String* results,
    int results_count,
    int* results_filled,
    bool skip_empty,
    bool init_results)
{
    // Make sure the required inputs aren't NULL.
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(separator);

    // results only needs to be initialized when results_count is > 0.
    // On 0, it doesn't matter what state it is.
    // On a negative number, results will be allocated by the function.
    if(results_count > 0)
        SSO_STRING_ASSERT_ARG(results);

    // Nothing needs to happen if there is no room for results. Return immediately.
    if(results_count == 0) {
        *results_filled = 0;
        return results;
    }

    // Determine if the results array needs to be allocated. If so, create an
    // initial buffer that has space for two items. This is the starting space since
    // splitting small items tends to be pretty common (e.g. an array of key-value pairs).
    bool allocate_results = results_count < 0;
    if(allocate_results) {
        results_count = 2;
        results = sso_string_malloc(results_count * sizeof(*results));
        if(!results)
            return NULL;

        // The results always have to be initialized by this function if the results array 
        // was also created by it. UB otherwise.
        init_results = true;
    }

    int count = 0;
    size_t start = 0;
    size_t size = string_size(str);

    while(true) {
        // Find the next instance of the separator. If the index is SIZE_MAX,
        // the function reached the end of the string without finding it.
        size_t next = sso_string_find_impl(str, start, separator, separator_length);
        if(next == SIZE_MAX)
            next = size;

        size_t copy_length = next - start;
        if(copy_length != 0 || !skip_empty) {
            // Add the string segment to the results array. If the value fails to be added,
            // there was an allocation error, so just break out of the function right away.
            if(init_results) {
                if(!string_init_size(results + count, string_data(str) + start, copy_length))
                    goto error;
            } else {
                if(!string_append_string_part(results + count, str, start, copy_length))
                    goto error;
            }

            if(++count == results_count) {
                if(allocate_results) {
                    results_count *= 2;
                    void* array = sso_string_realloc(results, results_count * sizeof(*results));
                    if(!array)
                        goto error;

                    results = array;
                } else {
                    *results_filled = count;
                    return results;
                }
            }
        }

        start = next + separator_length;

        // This indicates that the end of the string has been reached, and 
        // that all relevant substrings have been copied into the results array.
        if(start >= size) {
            *results_filled = count;
            return results;
        }
    }

    error:
        if(allocate_results)
            sso_string_free(results);

        *results_filled = 0;
        return NULL;
}

SSO_STRING_EXPORT String** sso_string_split_refs_impl(
    const String* str,
    const char* separator,
    size_t separator_length,
    String** results,
    int results_count,
    int* results_filled,
    bool skip_empty,
    bool allocate_results) 
{
    // Make sure the required inputs aren't NULL.
    SSO_STRING_ASSERT_ARG(str);
    SSO_STRING_ASSERT_ARG(separator);

    // results only needs to be initialized when results_count is > 0.
    // On 0, it doesn't matter what state it is.
    // On a negative number, results will be allocated by the function.
    if(results_count > 0)
        SSO_STRING_ASSERT_ARG(results);

    // Nothing needs to happen if there is no room for results. Return immediately.
    if(results_count == 0) {
        *results_filled = 0;
        return results;
    }

    // Determine if the results array needs to be allocated. If so, create an
    // initial buffer that has space for two items. This is the starting space since
    // splitting small items tends to be pretty common (e.g. an array of key-value pairs).
    allocate_results |= results_count < 0;
    if(allocate_results) {
        results_count = 2;
        results = sso_string_malloc(results_count * sizeof(*results));
        if(!results)
            return NULL;

        // The results always have to be initialized by this function if the results array 
        // was also created by it. UB otherwise.
        allocate_results = true;
    }

    int count = 0;
    size_t start = 0;
    size_t size = string_size(str);

    while(true) {
        // Find the next instance of the separator. If the index is SIZE_MAX,
        // the function reached the end of the string without finding it.
        size_t next = sso_string_find_impl(str, start, separator, separator_length);
        if(next == SIZE_MAX)
            next = size;

        size_t copy_length = next - start;
        if(copy_length != 0 || !skip_empty) {
            // Add the string segment to the results array. If the value fails to be added,
            // there was an allocation error, so just break out of the function right away.
            if(allocate_results) {
                String* slice = string_create_ref("");
                if(!slice)
                    goto error;
                results[count] = slice;
            }

            if(!string_append_string_part(results[count], str, start, copy_length))
                goto error;

            if(++count == results_count) {
                if(allocate_results) {
                    results_count *= 2;
                    void* array = sso_string_realloc(results, results_count * sizeof(*results));
                    if(!array)
                        goto error;

                    results = array;
                } else {
                    *results_filled = count;
                    return results;
                }
            }
        }

        start = next + separator_length;

        // This indicates that the end of the string has been reached, and 
        // that all relevant substrings have been copied into the results array.
        if(start >= size) {
            *results_filled = count;
            return results;
        }
    }

    error:
        if(allocate_results) {
            for(size_t i = 0; i < count; i++)
                string_free(results[i]);

            sso_string_free(results);
        }

        *results_filled = 0;
        return NULL;
}

SSO_STRING_EXPORT String* string_format_string(String* result, const String* format, ...) {
    va_list argp;

    va_start(argp, format);

    result = string_format_args_cstr(result, string_data(format), argp);

    va_end(argp);

    return result;
}

SSO_STRING_EXPORT String* string_format_cstr(String* result, const char* format, ...) {
    va_list argp;

    va_start(argp, format);

    result = string_format_args_cstr(result, format, argp);

    va_end(argp);

    return result;
}

SSO_STRING_EXPORT String* string_format_args_string(String* result, const String* format, va_list argp) {
    return string_format_args_cstr(result, string_data(format), argp);
}

#define SSO_STRING_FORMAT_BUFFER_SIZE 256

SSO_STRING_EXPORT String* string_format_args_cstr(String* result, const char* format, va_list argp) {
    // Define a stack allocated buffer so that we can avoid unnecessary
    // allocations when possible. The buffer is a static thread_local
    // one when available to avoid having to create it each call, 
    // otherwise it just creates a local variable.

    #ifdef SSO_THREAD_LOCAL

    static SSO_THREAD_LOCAL char buffer[SSO_STRING_FORMAT_BUFFER_SIZE];

    #else

    char buffer[SSO_STRING_FORMAT_BUFFER_SIZE];

    #endif

    SSO_STRING_ASSERT_ARG(format);

    size_t original_size = SIZE_MAX;
    if(!result) {
        result = string_create_ref("");
        if(!result)
            return NULL;
        
    } else {
        original_size = string_size(result);
    }

    // argp may have to be used again (if the result of vsnprint is > 256), so
    // make a copy of it to be used with the initial format.
    va_list copy;
    va_copy(copy, argp);

    int written = vsnprintf(buffer, SSO_STRING_FORMAT_BUFFER_SIZE, format, copy);
    va_end(copy);

    if(written < 0) {
        goto error;
    } else if(written < SSO_STRING_FORMAT_BUFFER_SIZE) {
        if(!string_append_cstr(result, buffer))\
            goto error;
        return result;
    } else {
        if(!string_reserve(result, string_size(result) + written))
            goto error;

        written = vsnprintf(
            string_cstr(result) + string_size(result),
            written + 1,
            format,
            argp);

        if(written < 0)
            goto error;

        size_t new_size = original_size == SIZE_MAX ? written : original_size + written;

        // The string should always be long at this point, but do the check just in case.
        if(sso_string_is_long(result))
            sso_string_long_set_size(result, new_size);
        else
            sso_string_short_set_size(result, new_size);
    }

    return result;

    error:
        if(original_size == SIZE_MAX)
            string_free(result);
        else {
            string_cstr(result)[original_size] = '\0';
            sso_string_set_size(result, original_size);
        }

        return NULL;
}

SSO_STRING_EXPORT String* string_format_time_cstr(String* result, const char* format, const struct tm* timeptr) {
    #ifdef SSO_THREAD_LOCAL

    static SSO_THREAD_LOCAL char buffer[SSO_STRING_FORMAT_BUFFER_SIZE];

    #else

    char buffer[SSO_STRING_FORMAT_BUFFER_SIZE];

    #endif

    SSO_STRING_ASSERT_ARG(format);

    size_t original_size = SIZE_MAX;
    if(!result) {
        result = string_create_ref("");
        if(!result)
            return NULL;
        
    } else {
        original_size = string_size(result);
    }

    size_t written = strftime(buffer, SSO_STRING_FORMAT_BUFFER_SIZE, format, timeptr);

    if(written != 0) {
        if(!string_append_cstr_part(result, buffer, 0, written))
            goto error;
    } else {
        int count = 1;
        size_t offset = original_size == SIZE_MAX ? 0 : original_size;
        while(written == 0) {
            count++;
            size_t blank_space = SSO_STRING_FORMAT_BUFFER_SIZE * count;
            if(!string_reserve(result, offset + blank_space))
                goto error;
            
            written = strftime(string_cstr(result) + offset, blank_space, format, timeptr);
        }
        sso_string_set_size(result, offset + written);
        string_cstr(result)[offset + written] = '\0';
    }

    return result;

    error:
        if(original_size == SIZE_MAX)
            string_free(result);
        else
            string_cstr(result)[original_size] = '\0';

        return NULL;
}

SSO_STRING_EXPORT struct StringFileReadState* string_file_read_state_create(int buffer_size) {
    char* buffer = sso_string_malloc(buffer_size);
    if(!buffer)
        return NULL;

    struct StringFileReadState* read_state = sso_string_malloc(sizeof(*read_state));
    if(!read_state) {
        sso_string_free(buffer);
        return NULL;
    }

    string_file_read_state_init(read_state, buffer, buffer_size);

    return read_state;
}

SSO_STRING_EXPORT void string_file_read_state_init(struct StringFileReadState* read_state, char* buffer, int buffer_size) {
    read_state->buffer = buffer;
    read_state->buffer_size = buffer_size;
    read_state->new_line_index = -1;
    read_state->max = 0;
    read_state->result = STRING_FILE_READ_RESULT_SUCCESS;
}

SSO_STRING_EXPORT void string_file_read_state_free(struct StringFileReadState* read_state) {
    if(read_state)
        sso_string_free(read_state->buffer);
    sso_string_free(read_state);
}

static bool sso_string_read_loaded_file_buffer(String* str, FILE* file, struct StringFileReadState* context) {
    // The starting index should be the index of the last new line character plus one so that
    // the \n itself is skipped. new_line_index will be -1 when reading from the start of the
    // buffer to account for this.
    int start = context->new_line_index + 1;
    char* end = memchr(context->buffer + start, '\n', context->max - start);
    if(!end) {
        // No new line was found. Append the entire buffer from the starting index.
        if(!string_append_cstr_part(str, context->buffer, start, context->max - start)) {
            context->result = STRING_FILE_READ_RESULT_OUT_OF_MEMORY;
            return false;
        }

        if(feof(file)) {
            context->result = STRING_FILE_READ_RESULT_EOF;

            // Set new_line_index to -2 to make breaking out of the read loop easier
            // since it checks if new_line_index == -1.
            context->new_line_index = -2;
        } else {
            context->new_line_index = -1;
        }
        
    } else {
        // A new line was found. Append the buffer from the last new line to the next new line.
        // If the line breaks were next to each other, str will be an empty string.
        int new_line_index = end - context->buffer;
        if(!string_append_cstr_part(str, context->buffer, start, new_line_index - start)) {
            context->result = STRING_FILE_READ_RESULT_OUT_OF_MEMORY;
            return false;
        }

        context->new_line_index = new_line_index;
    }

    return true;
}

SSO_STRING_EXPORT bool string_file_read_line(String* str, FILE* file, struct StringFileReadState* context) {
    // Clear the string to get ready to read the next line of the file.
    string_clear(str);

    // If the file has reached its end and the result has been set,
    // there is nothing left to read and we can exit early.
    if(feof(file) && context->result != STRING_FILE_READ_RESULT_SUCCESS) {
        return false;
    }

    bool load_success = false;

    do {
        // Read more data if the entire buffer has been read.
        // Either there was no new line after the last read (context->new_line_index == -1)
        // or the new line was at the end of the buffer.
        if(context->new_line_index == -1 || context->new_line_index + 1 >= context->max) {
            context->max = fread(context->buffer, 1, context->buffer_size, file);
            
            // If the buffer wasn't filled, make sure it was just because of EOF and
            // not an error.
            if(context->max < context->buffer_size && ferror(file)) {
                context->result = STRING_FILE_READ_RESULT_ERROR;
                return false;
            }

            // If no data was read, set the result to EOF and return.
            // It may be possible that no data was read even though it wasn't
            // the EOF (for example, when reading from a network stream), but
            // for our cases describing it as EOF is good enough for now.
            if(context->max == 0) {
                context->result = STRING_FILE_READ_RESULT_EOF;

                // Return load_success since if any data has been previously
                // read it should count as a successful operation.
                return load_success;
            }

            // Set the new_line_index to -1 to indicate that the read needs to start
            // from the beginning of the buffer.
            context->new_line_index = -1;
        }

        load_success = sso_string_read_loaded_file_buffer(str, file, context);
    }
    while(load_success && context->new_line_index == -1);
    // ^^^
    // Check if new_line_index is -1 because that means the end of the buffer has been read
    // without finding any new lines from the starting index and we need to read another chunk to
    // find more.

    return load_success;
}

SSO_STRING_EXPORT bool string_file_read_all(String* str, FILE* file) {
    long current = ftell(file);
    if(current < 0)
        return false;

    if(fseek(file, 0, SEEK_END) != 0)
        return false;
    
    long size = ftell(file);
    if(size < 0)
        return false;

    if(fseek(file, current, SEEK_SET) != 0)
        return false;

    if(!string_reserve(str, size))
        return false;
    
    char* buffer = string_cstr(str);
    size_t read = fread(buffer, 1, size, file);
    if(read != size)
        return false;

    buffer[size] = '\0';
    sso_string_set_size(str, size);

    return true;
}