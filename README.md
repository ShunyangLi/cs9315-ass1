- need convert string into lowercase before email_in function
  - `void lower(char str[])`
- `email_recv and email_send` are not necessary need to delete them
  - `delete this two function`
- the string of email address max length is 512
  - `strlen(str) <= 512 + 1`
- **need to use flexible size of string** otherwise just can get 7/12 marks
  - use dynamic calloc for the string
- also need to write other operations

#### when use dynamic calloc for a char pointer we can do like this

```c
// the struct of email address
typedef struct EmailAddr {
   char    *local;
   char    *domain;
}        EmailAddr;

// assume this comes from fronend
char em[] = "Shuyang.Li@ysw.ed-a.A";
char *local = strtok(em, "@");
char *domain = strtok(NULL, "@");

lower_string(local);
lower_string(domain)

EmailAddr  *result = (EmailAddr *) calloc(1, sizeof(EmailAddr));
// plus one for \0
result->local = (char *) calloc(1, strlen(local)+1);
result->domain = (char *) calloc(1, strlen(domain)+1);

// copy the local and domain into the struct
strcpy(result->local, local);
strcpy(result->domain, domain);

// and the result is 
// shuyang.li
// ysw.ed-a.a
```

```c
// this is convert a uppercase into lowercase
void lower(char str[]) {
   int i = 0;
   for (i = 0; str[i] != '\0'; i ++) {
      if (str[i] >= 'A' && str[i] <= 'Z') {
         str[i] = str[i] + 32;
      }
   }
}
```

- about how to check local and domain part in c, and gonna to have a little change

  ```c
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
  ```

  ```c
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
  ```

  