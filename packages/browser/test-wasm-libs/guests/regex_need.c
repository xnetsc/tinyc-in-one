/* PCRE2 8-bit surface, static-dependency form. */
#include <stdio.h>
#include <string.h>
#include "guest_check.h"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

static pcre2_code *compile(const char *pat, uint32_t options, int *errcode) {
  PCRE2_SIZE erroffset;
  return pcre2_compile((PCRE2_SPTR)pat, PCRE2_ZERO_TERMINATED, options,
                       errcode, &erroffset, NULL);
}

static int match(pcre2_code *code, const char *subject, pcre2_match_data **out_md) {
  pcre2_match_data *md = pcre2_match_data_create_from_pattern(code, NULL);
  int rc = pcre2_match(code, (PCRE2_SPTR)subject, strlen(subject), 0, 0, md, NULL);
  *out_md = md;
  return rc;
}

int main(void) {
  int errcode = 0;

  /* Date pattern with three capture groups. */
  {
    pcre2_code *code = compile("(\\d{4})-(\\d{2})-(\\d{2})", 0, &errcode);
    CK("compile_date", code != NULL);
    pcre2_match_data *md = NULL;
    int rc = match(code, "date: 2026-11-28 end", &md);
    CK("match_date_rc", rc == 4);
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
    CK("ov_whole", ov[0] == 6 && ov[1] == 16);
    CK("ov_group1", ov[2] == 6 && ov[3] == 10);
    CK("ov_count", pcre2_get_ovector_count(md) == 4);
    unsigned char *sub = NULL;
    PCRE2_SIZE sublen = 0;
    CK("substring_rc", pcre2_substring_get_bynumber(md, 2, &sub, &sublen) == 0);
    CK("substring_val", sublen == 2 && memcmp(sub, "11", 2) == 0);
    pcre2_substring_free(sub);
    uint32_t count = 0;
    CK("info_rc", pcre2_pattern_info(code, PCRE2_INFO_CAPTURECOUNT, &count) == 0);
    CK("info_capturecount", count == 3);
    pcre2_match_data_free(md);
    pcre2_code_free(code);
  }

  /* Named groups, by name and by table. */
  {
    pcre2_code *code = compile("(?P<year>\\d{4})-(?<mon>\\d{2})", 0, &errcode);
    CK("compile_named", code != NULL);
    pcre2_match_data *md = NULL;
    int rc = match(code, "x2026-11y", &md);
    CK("match_named_rc", rc == 3);
    unsigned char *sub = NULL;
    PCRE2_SIZE sublen = 0;
    CK("substring_byname_rc", pcre2_substring_get_byname(md, (PCRE2_SPTR)"mon", &sub, &sublen) == 0);
    CK("substring_byname", sublen == 2 && memcmp(sub, "11", 2) == 0);
    pcre2_substring_free(sub);
    uint32_t namecount = 0;
    CK("info_namecount", pcre2_pattern_info(code, PCRE2_INFO_NAMECOUNT, &namecount) == 0
       && namecount == 2);
    pcre2_match_data_free(md);
    pcre2_code_free(code);
  }

  /* Options and semantics. */
  {
    pcre2_code *code = compile("ABC", PCRE2_CASELESS, &errcode);
    pcre2_match_data *md = NULL;
    CK("caseless", match(code, "xxabcxx", &md) == 1);
    pcre2_match_data_free(md);
    pcre2_code_free(code);

    code = compile("a.c", 0, &errcode);
    CK("dot_no_newline", match(code, "a\nc", &md) == PCRE2_ERROR_NOMATCH);
    pcre2_match_data_free(md);
    CK("dot_matches_cr", match(code, "a\rc", &md) == 1);
    pcre2_match_data_free(md);
    pcre2_code_free(code);

    code = compile("^\\w+$", PCRE2_MULTILINE, &errcode);
    CK("multiline_anchor", match(code, "!!\nabc\n??", &md) == 1);
    pcre2_match_data_free(md);
    pcre2_code_free(code);

    code = compile("(a)|(b)", 0, &errcode);
    int rc = match(code, "b", &md);
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
    CK("unset_pair", rc == 3 && ov[2] == PCRE2_UNSET);
    unsigned char *sub = NULL;
    PCRE2_SIZE sublen = 0;
    CK("unset_substring", pcre2_substring_get_bynumber(md, 1, &sub, &sublen)
       == PCRE2_ERROR_UNSET);
    CK("nosubstring", pcre2_substring_get_bynumber(md, 9, &sub, &sublen)
       == PCRE2_ERROR_NOSUBSTRING);
    pcre2_match_data_free(md);
    pcre2_code_free(code);
  }

  /* Unsupported constructs fail compilation, named. */
  {
    pcre2_code *code = compile("(?>a)b", 0, &errcode);
    CK("atomic_refused", code == NULL && errcode == 900);
    unsigned char buf[256];
    int n = pcre2_get_error_message(errcode, buf, sizeof(buf));
    CK("atomic_named", n > 0 && strstr((char *)buf, "atomic") != NULL);

    code = compile("a\\Kb", 0, &errcode);
    CK("keepout_refused", code == NULL && errcode == 902);
    code = compile("a++", 0, &errcode);
    CK("possessive_refused", code == NULL && errcode == 901);
    code = compile("\\p{L}", 0, &errcode);
    CK("property_refused", code == NULL && errcode == 908);
  }

  DONE();
}
