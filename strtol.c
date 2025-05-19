#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

int convert_char_to_digit(const char* p, bool* is_valid_digit)
{
    if (*p >= '0' && *p <= '9') {
        return *p - '0';
    } else if (*p >= 'a' && *p <= 'z') {
        return *p - 'a' + 10;
    } else if (*p >= 'A' && *p <= 'Z') {
        return *p - 'A' + 10;
    } else {
        *is_valid_digit = false;
        return 0;
    }
}

void skip_whitespaces(const char** p)
{
    while (**p != '\0' && isspace((unsigned char)**p)) {
        (*p)++;
    }
}

int detect_sign_and_align_p(const char** p)
{
    bool is_negative = false;
    if (**p == '+') {
        (*p)++;
    } else if (**p == '-') {
        is_negative = true;
        (*p)++;
    }
    return is_negative ? -1 : 1;
}

int detect_base_and_align_p(int base, const char** p)
{
	char first_char = **p;
	char second_char = (*p)[1];

	if (base == 0) {
		if (first_char == '0') {
			if (tolower(second_char) == 'x') {
				base = 16;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	}

	// align p position only if a valid digit follows prefix
	if (base == 16 && first_char == '0' && tolower(second_char) == 'x') {
		bool is_valid_digit = true;
		int digit = convert_char_to_digit(*p + 2, &is_valid_digit);
		if (is_valid_digit && digit < base) {
			*p += 2; // skip "0x"
		}
	} else if (base == 8 && first_char == '0') {
		char third_char = (*p)[1];
		if (third_char >= '0' && third_char <= '7') {
			*p += 1; // skip '0'
		}
	}

	return base;
}


bool check_overflow_and_update(long value, int base, int digit, int sign, long* new_value)
{
    if (sign > 0) {
        long threshold = (LONG_MAX - digit) / base;
        if (value > threshold) {
            *new_value = LONG_MAX;
            errno = ERANGE;
            return true;
        }
        *new_value = value * base + digit;
        return false;
    } else {
        long threshold = (LONG_MIN + digit) / base;
        if (value < threshold) {
            *new_value = LONG_MIN;
            errno = ERANGE;
            return true;
        }
        *new_value = value * base - digit;
        return false;
    }
}

void process_digits(const char** p, long* converted_value, int base, int sign,
                    bool* has_digits, bool* is_overflowed)
{
    while (**p != '\0') {
        bool is_valid = true;
        int digit = convert_char_to_digit(*p, &is_valid);
        if (!is_valid || digit >= base) 
			break;

        *has_digits = true;
        long updated;

        if (check_overflow_and_update(*converted_value, base, digit, sign, &updated)) {
            *is_overflowed   = true;
            *converted_value = updated;
            (*p)++;
            while (**p != '\0') {
                digit = convert_char_to_digit(*p, &is_valid);
                if (!is_valid || digit >= base)
					break;
					
                (*p)++;
            }
            return;
        }

        *converted_value = updated;
        (*p)++;
    }
}

void handle_no_digits(const char* nptr, const char** p, long* converted_value, bool has_digits)
{
    if (!has_digits) {
        *converted_value = 0;
        *p = nptr;
    }
}

long strtol(const char* nptr, char** endptr, int base)
{
    if (base != 0 && (base < 2 || base > 36)) {
        errno = EINVAL;
        if (endptr) *endptr = (char*)nptr;
        return 0;
    }

    const char* p = nptr;
    long converted_value = 0;
    bool has_digits = false;
    bool is_overflowed = false;
    int sign = 1;

    skip_whitespaces(&p);
    sign = detect_sign_and_align_p(&p);
    base = detect_base_and_align_p(base, &p);
    
    process_digits(&p, &converted_value, base, sign, &has_digits, &is_overflowed);
    handle_no_digits(nptr, &p, &converted_value, has_digits);

    if (NULL != endptr)
		*endptr = (char*)p;
		
    return converted_value;
}
