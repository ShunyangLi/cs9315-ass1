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


#define MAX_SIZE 256
#define True 1
#define False 0



PG_MODULE_MAGIC;

typedef struct Email
{
	char        local[MAX_SIZE];
    char		domain[MAX_SIZE];
}			Email;

static int check_at(char *str);
static int isInvalidDomain(char *domain);
static int isInvalidLocal(char *local);

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(email_in);

Datum
email_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
    char       emailAddr[MAX_SIZE*2];
	Email      *result;

    strcpy(emailAddr, str);

	if (check_at(emailAddr) != 1) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for email: \"%s\"",
                               str)));
	} else {
	    char *local = strtok(emailAddr, "@");
	    char *domain = strtok(NULL, "@");

	    if (isInvalidLocal(local) && isInvalidDomain(domain)) {
            result = (Email *) palloc(sizeof(Email));

            strcpy(result->local, local);
            strcpy(result->domain, domain);
	    } else {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                            errmsg("invalid input syntax for email: \"%s\"",
                                   str)));
	    }
	}


	PG_RETURN_POINTER(result);
}



/**
 * check how many @ sign of email address
 * @param str the email address
 * @return the number of @ sign
 */
static int check_at(char *str) {
    int num = 0, i = 0;

    for (i = 0; i < strlen(str); i ++) {
        if (str[i] == '@') {
            num ++;
        }
    }

    return num;
}

/**
 * use regex to match the correct domain format
 * @param domain the domain email
 * @return if the domain regex is correct then return true
 *          otherwise return false
 */
static int isInvalidDomain(char *domain) {
    // the flag of check is invalid
    int isInvalid = False;

    regex_t domain_match;
    char *domain_regex = "^[a-zA-Z]+(\\-[a-zA-Z0-9]+)*[0-9]*\\."
                         "[a-zA-Z]+(\\-[a-zA-Z0-9]+)*[0-9]*"
                         "(\\.([a-zA-Z]+(\\-+[a-zA-Z0-9]+)*[0-9]*)+)*$";
    regcomp(&domain_match, domain_regex, REG_EXTENDED);

    if (!regexec(&domain_match, domain, 0, NULL, 0)) isInvalid = True;
    // free the regex
    regfree(&domain_match);

    return isInvalid;

}

/**
 * use regex to match the correct domain format
 * @param local the local of email
 * @return if the local of email is correct return true
 *          otherwise return false
 */
static int isInvalidLocal(char *local) {
    // the flag of check is invalid
    int isInvalid = False;

    regex_t local_match;
    char *local_regex = "^[a-zA-Z]+(\\-[a-zA-Z0-9]+)*[0-9]*"
                        "(\\.([a-zA-Z]+(\\-+[a-zA-Z0-9]+)*[0-9]*)+)*$";

    regcomp(&local_match, local_regex, REG_EXTENDED);

    if (!regexec(&local_match, local, 0, NULL, 0)) isInvalid = True;
    // free the regex
    regfree(&local_match);

    return isInvalid;
}


PG_FUNCTION_INFO_V1(email_out);

Datum
email_out(PG_FUNCTION_ARGS)
{
	Email      *email = (Email *) PG_GETARG_POINTER(0);
	char	   *result;

	result = (char *) palloc(sizeof(Email));
    snprintf(result, sizeof(Email), "%s@%s", email->local, email->domain);

    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(email_recv);

Datum
email_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	Email    *result;

	result = (Email *) palloc(sizeof(Email));

    strcpy(result->local, pq_getmsgstring(buf));
    strcpy(result->domain, pq_getmsgstring(buf));

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(email_send);

Datum
email_send(PG_FUNCTION_ARGS)
{
	Email    *email = (Email *) PG_GETARG_POINTER(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
    pq_sendstring(&buf, email->local);
    pq_sendstring(&buf, email->domain);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
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
email_compare(Email * a, Email * b)
{
	char *email_a, *email_b;
    email_a = (char *) palloc(sizeof(Email));
    snprintf(email_a, sizeof(Email), "%s@%s", a->local, a->domain);

    email_b = (char *) palloc(sizeof(Email));
    snprintf(email_b, sizeof(Email), "%s@%s", b->local, b->domain);

    int res = strcmp(email_a, email_b);
    pfree(email_a);
    pfree(email_b);

    return res;
}


PG_FUNCTION_INFO_V1(email_lt);

Datum
email_lt(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
    Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) < 0);
}

PG_FUNCTION_INFO_V1(email_le);

Datum
email_le(PG_FUNCTION_ARGS)
{
    Email    *a = (Email *) PG_GETARG_POINTER(0);
    Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(email_eq);

Datum
email_eq(PG_FUNCTION_ARGS)
{
    Email    *a = (Email *) PG_GETARG_POINTER(0);
    Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) == 0);
}

PG_FUNCTION_INFO_V1(email_ge);

Datum
email_ge(PG_FUNCTION_ARGS)
{
    Email    *a = (Email *) PG_GETARG_POINTER(0);
    Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(email_gt);

Datum
email_gt(PG_FUNCTION_ARGS)
{
    Email    *a = (Email *) PG_GETARG_POINTER(0);
    Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(email_compare(a, b) > 0);
}

PG_FUNCTION_INFO_V1(email_cmp);

Datum
email_cmp(PG_FUNCTION_ARGS)
{
    Email    *a = (Email *) PG_GETARG_POINTER(0);
    Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(email_compare(a, b));
}
