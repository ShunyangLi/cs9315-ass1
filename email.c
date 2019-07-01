/*
 * src/tutorial/email.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include <string.h>
#include <regex.h>


#define MAX_SIZE 513
#define True 1
#define False 0



PG_MODULE_MAGIC;

typedef struct EmailAddr
{
    int32       length;
	char        emailAddr[0];
}			EmailAddr;

static int isInvalidEmailAddr(char *emailAddr);
static void lower(char str[]);
Datum hash_any(unsigned char *k, int keylen);
static int same_domain(EmailAddr * a, EmailAddr * b);
static int email_compare(EmailAddr * a, EmailAddr * b);

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(email_in);

Datum
email_in(PG_FUNCTION_ARGS)
{
    EmailAddr  *result;
    int strLen = strlen(PG_GETARG_CSTRING(0));

    if (strLen <= MAX_SIZE) {
        char email[strLen];
        strcpy(email, PG_GETARG_CSTRING(0));

        if (isInvalidEmailAddr(email)) {
            // convert the email address into lowercase
            lower(email);
            result = (EmailAddr *) palloc(VARHDRSZ + strlen(email)+1);
            SET_VARSIZE(result, VARHDRSZ + strlen(email)+1);

            memmove(result->emailAddr, email, strlen(email)+1);

        } else {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                            errmsg("invalid input syntax for email address: \"%s\"",
                                   email)));
        }

    } else {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("the email address length of \"%s\" is invalid",
                               PG_GETARG_CSTRING(0))));
    }

	PG_RETURN_POINTER(result);
}



/**
 * use regex to match the correct email format
 * @param emailAddr the email address
 * @return if the local and domain regex is correct then return true
 *          otherwise return false
 */
static int isInvalidEmailAddr(char *emailAddr) {
    // the flag of check is invalid
    int isInvalid = False;
    regex_t email_match;

    // the regular expression
    char *email_regex = "^[a-zA-Z]+(\\-[a-zA-Z0-9]+)*[a-zA-Z0-9]*"
                         "(\\.([a-zA-Z]+(\\-+[a-zA-Z0-9]+)*[0-9]*)+)*"
                         "@[a-zA-Z]+(\\-[a-zA-Z0-9]+)*[a-zA-Z0-9]*"
                         "\\.[a-zA-Z]+(\\-[a-zA-Z0-9]+)*[0-9]*"
                         "(\\.([a-zA-Z]+(\\-+[a-zA-Z0-9]+)*[0-9]*)+)*$";

    regcomp(&email_match, email_regex, REG_EXTENDED);

    if (!regexec(&email_match, emailAddr, 0, NULL, 0)) isInvalid = True;
    // free the regex
    regfree(&email_match);

    return isInvalid;

}

// convert string into lowercase
static void lower(char str[]) {
    int i = 0;
    for (i = 0; str[i] != '\0'; i ++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] + 32;
        }
    }
}


PG_FUNCTION_INFO_V1(email_out);

Datum
email_out(PG_FUNCTION_ARGS)
{
	EmailAddr  *email = (EmailAddr *) PG_GETARG_POINTER(0);
	char	   *result;

    result = psprintf("%s", email->emailAddr);
    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

//#define Mag(c)	((c)->x*(c)->x + (c)->y*(c)->y)

static int
email_compare(EmailAddr * a, EmailAddr * b)
{

    char emailA[strlen(a->emailAddr)+1];
    char emailB[strlen(b->emailAddr)+1];
    strcpy(emailA, a->emailAddr);
    strcpy(emailB, b->emailAddr);

    char *localA = strtok(emailA, "@");
    char *domainA = strtok(NULL,"@");

    char *localB = strtok(emailB, "@");
    char *domainB = strtok(NULL, "@");

    if (strcasecmp(domainA, domainB) != 0) {
        return strcasecmp(domainA, domainB);
    }

    return strcasecmp(localA, localB);
}

static int
same_domain(EmailAddr * a, EmailAddr * b)
{
    char emailA[strlen(a->emailAddr)+1];
    char emailB[strlen(b->emailAddr)+1];
    strcpy(emailA, a->emailAddr);
    strcpy(emailB, b->emailAddr);

    char *domainA = strtok(emailA, "@");
    domainA = strtok(NULL,"@");
    char *domainB = strtok(emailB, "@");
    domainB = strtok(NULL, "@");

    return strcasecmp(domainA, domainB);
}


PG_FUNCTION_INFO_V1(email_lt);
// <
Datum
email_lt(PG_FUNCTION_ARGS)
{
	EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) < 0);
}

PG_FUNCTION_INFO_V1(email_le);
// <=
Datum
email_le(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(email_eq);
// =
Datum
email_eq(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) == 0);
}

PG_FUNCTION_INFO_V1(email_ge);
// >=
Datum
email_ge(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(email_gt);
// >
Datum
email_gt(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) > 0);
}

PG_FUNCTION_INFO_V1(email_cmp);

Datum
email_cmp(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(email_compare(a, b));
}


PG_FUNCTION_INFO_V1(email_ne);
// <>
Datum
email_ne(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(email_compare(a, b) != 0);
}


PG_FUNCTION_INFO_V1(email_sd);
// ~
Datum
email_sd(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(same_domain(a, b) == 0);
}



PG_FUNCTION_INFO_V1(email_nsd);
// !~
Datum
email_nsd(PG_FUNCTION_ARGS)
{
    EmailAddr    *a = (EmailAddr *) PG_GETARG_POINTER(0);
    EmailAddr    *b = (EmailAddr *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(same_domain(a, b) != 0);
}


PG_FUNCTION_INFO_V1(email_hash_index);

Datum
email_hash_index(PG_FUNCTION_ARGS)
{
    EmailAddr    *email = (EmailAddr *) PG_GETARG_POINTER(0);
    char	     *emailAddr;
    int32        res;

    emailAddr = psprintf("%s", email->emailAddr);

    res = hash_any((unsigned char*) emailAddr, strlen(emailAddr));
    // free the email
    pfree(emailAddr);

    PG_RETURN_INT32(res);
}

