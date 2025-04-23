#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

int convert_char_to_digit(const char* p, bool* is_valid_digit)
{
	int digit;
	if (*p >= '0' && *p <= '9') {
		digit = *p - '0';
	}
	else if (*p >= 'a' && *p <= 'z') {
		digit = *p - 'a' + 10;
	}
	else if (*p >= 'A' && *p <= 'Z') {
		digit = *p - 'A' + 10;
	}
	else {
		*is_valid_digit = false;
		return 0;
	}
	return digit;
}
void skip_whitespaces(const char** p)
{
	while (**p != '\0' && isspace(**p)) {
		(*p)++;
	}
}

int detect_sign_and_align_p(const char** p)
{
	bool is_negative = false;
	if (**p == '+') {
		is_negative = false;
		(*p)++;
	}
	if (**p == '-') {
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
			}
			else {
				base = 8;
			}
		}
		else {
			base = 10;
		}
	}

	// align p position
	if (base == 16 && first_char == '0' && tolower(second_char) == 'x') {
		*p += 2; // We skip "0x"
	}
	else if (base == 8 && first_char == '0') {
		*p += 1; // We skip '0'
	}

	return base;
}

void skip_to_first_non_digit(const char** p, int base) {
	(*p)++;
	while (**p != '\0') {
		bool is_valid_digit = true;
		int digit = convert_char_to_digit(*p, &is_valid_digit);
		if (!is_valid_digit || digit >= base) {
			break;
		}
		(*p)++;
	}
}

bool multiply_and_check_overflow(long value, int base, long* result)
{
	// check upper and lower bound for multiplication:
	// if value*base > LONG_MAX or value*base < LONG_MIN,
	// result would wrap around modulo 2^n, so we detect it before
	if (value >  0 && value > LONG_MAX / base) return true;
	if (value <  0 && value < LONG_MIN / base) return true;
	*result = value * base;
	return false;
}
bool add_and_check_overflow(long value, int sign, int digit, long* result)
{
	// pre-check addition:
	// adding sign*digit to value past LONG_MAX/LONG_MIN
	// would wrap the result to the opposite end, so detect before arithmetic
	if (sign > 0 && value > LONG_MAX - digit) return true;
	if (sign < 0 && value < LONG_MIN + digit) return true;
	*result = value + sign * digit;
	return false;
}

bool process_digit(const char** p, long* converted_value, int base, int sign, bool* is_overflowed) {
    bool is_valid_digit = true;
    int digit = convert_char_to_digit(*p, &is_valid_digit);

    if (!is_valid_digit || digit >= base) {
        return false; // Not a valid digit for the given base
    }

    // Check for overflows and convert digit to its proper numeric system
    long temp;
    if (multiply_and_check_overflow(*converted_value, base, &temp) ||
        add_and_check_overflow(temp, sign, digit, converted_value)) {
        *is_overflowed = true;
        *converted_value = (sign > 0 ? LONG_MAX : LONG_MIN);
        return true; // (Overflow occurred)
    }

    (*p)++;
    return true;
}

void process_digits(const char** p, long* converted_value, int base, int sign,
					bool* has_digits, bool* is_overflowed)
{
	while (**p != '\0') {
		if (!process_digit(p, converted_value, base, sign, is_overflowed)) {
			break; // Not a valid digit for the given base
		}

		*has_digits = true;

		if (*is_overflowed) {
			skip_to_first_non_digit(p, base);
			break;
		}
	}
}
void handle_no_digits(const char* nptr, const char** p, long* converted_value, bool has_digits)
{
	if (!has_digits) {
		*converted_value = 0;
		*p = nptr; // Reset pointer position
	}
}

long strtol(const char* nptr, char** endptr, int base)
{
	if (base != 0 && (base < 2 || base > 36)) {
		errno = EINVAL;
		if (endptr)
			*endptr = (char*)nptr;
		return 0;
	}

	const char* p = nptr; // Pointer to the current character in the string
						  // Initialized to the same location as nptr as const
						  // (So we can modify p to iterate over string, but we
						  // cant modify the characters in the string)
	long converted_value = 0;
	bool has_digits = false;
	bool is_overflowed = false;
	int sign = 1;

	skip_whitespaces(&p);
	sign = detect_sign_and_align_p(&p);
	base = detect_base_and_align_p(base, &p);

	process_digits(&p, &converted_value, base, sign, &has_digits, &is_overflowed);
	handle_no_digits(nptr, &p, &converted_value, has_digits);

	if (endptr != NULL) {
		*endptr = (char*)p;
	}

	if (is_overflowed) {
		errno = ERANGE;
	}
	return converted_value;
}

int main()
{
	const char* str = "2137abcd";
	char* endptr;
	long result = strtol(str, &endptr, 16);
	printf("Converted value: %ld\n", result);
	printf("Remaining string: %s\n", endptr);
}
