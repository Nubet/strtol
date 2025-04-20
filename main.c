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

bool multiplication_overflow(long value, int base, long* result)
{
	// during multiplication overflow but on overflow the product wraps around
	// modulo 2^n so (temp/base) will not recover to the orginal value
	long temp = value * base;
	if (temp / base != value) {
		return true;
	}
	*result = temp;
	return false;
}

bool addition_overflow(long value, int sign, int digit, long* result)
{
	// during overflow the result would jump to the opposite end of the range
	long new_value = value + sign * digit;
	if ((sign > 0 && new_value < value) || (sign < 0 && new_value > value)) {
		return true;
	}
	*result = new_value;
	return false;
}

void process_digits(const char** p, long* converted_value, int base, int sign,
					bool* has_digits, bool* is_overflowed)
{
	while (**p != '\0') {
		bool is_valid_digit = true;
		int digit = convert_char_to_digit(*p, &is_valid_digit);

		if (!is_valid_digit || digit >= base)
			break;

		*has_digits = true;

		// Check for overflows
		long temp;
		if (multiplication_overflow(*converted_value, base, &temp) ||
			addition_overflow(temp, sign, digit, converted_value)) {
			*is_overflowed = true;
			*converted_value = (sign > 0 ? LONG_MAX : LONG_MIN);
			break;
		}

		(*p)++;
	}
}
void handle_no_digits(const char* nptr, const char** p, long* converted_value, bool has_digits)
{
	if (!has_digits) {
		*converted_value = 0;
		*p = nptr; // Reset pointer position
		skip_whitespaces(p);
		if (**p == '+' || **p == '-') {
			(*p)++;
		}
	}
}

long strtol(const char* nptr, char** endptr, int base)
{

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
	const char* str = "12345tralalala";
	char* endptr;
	long result = strtol(str, &endptr, 10);
	printf("Converted value: %ld\n", result);
	printf("Remaining string: %s\n", endptr);
}
