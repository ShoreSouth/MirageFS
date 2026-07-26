#include "msh_test_common.h"

void test_msh_cleanup_root(void)
{
    int rc;

    rc = system("rm -rf -- './miragefs.root'");
    (void)rc;
}

int test_msh_run_line(msh_context_t *ctx, const char *line)
{
    char line_buf[MSH_LINE_MAX];
    int written;

    written = snprintf(line_buf, sizeof(line_buf), "%s", line);
    TEST_ASSERT_TRUE((written >= 0) && ((size_t)written < sizeof(line_buf)));
    return msh_run_line(ctx, line_buf);
}

int test_msh_run_line_capture(msh_context_t *ctx, const char *line, char *buf,
                              size_t size)
{
    FILE *capture;
    int saved_stdout;
    int rc;
    size_t actual;

    TEST_ASSERT_TRUE(buf != NULL);
    TEST_ASSERT_TRUE(size > 0U);

    saved_stdout = dup(STDOUT_FILENO);
    TEST_ASSERT_TRUE(saved_stdout >= 0);
    capture = tmpfile();
    TEST_ASSERT_TRUE(capture != NULL);
    TEST_ASSERT_EQ_INT(fflush(stdout), 0);
    TEST_ASSERT_EQ_INT(dup2(fileno(capture), STDOUT_FILENO), STDOUT_FILENO);

    rc = test_msh_run_line(ctx, line);

    TEST_ASSERT_EQ_INT(fflush(stdout), 0);
    TEST_ASSERT_EQ_INT(dup2(saved_stdout, STDOUT_FILENO), STDOUT_FILENO);
    close(saved_stdout);
    rewind(capture);
    actual = fread(buf, 1U, size - 1U, capture);
    buf[actual] = '\0';
    fclose(capture);
    return rc;
}

int test_msh_run_line_with_stdin(msh_context_t *ctx, const char *line,
                                 const char *input)
{
    FILE *tmp;
    int saved_stdin;
    int rc;

    saved_stdin = dup(STDIN_FILENO);
    TEST_ASSERT_TRUE(saved_stdin >= 0);
    tmp = tmpfile();
    TEST_ASSERT_TRUE(tmp != NULL);
    if (input != NULL)
    {
        TEST_ASSERT_EQ_INT(fwrite(input, 1U, strlen(input), tmp),
                           strlen(input));
    }
    rewind(tmp);
    TEST_ASSERT_EQ_INT(dup2(fileno(tmp), STDIN_FILENO), STDIN_FILENO);
    clearerr(stdin);

    rc = test_msh_run_line(ctx, line);

    TEST_ASSERT_EQ_INT(dup2(saved_stdin, STDIN_FILENO), STDIN_FILENO);
    clearerr(stdin);
    close(saved_stdin);
    fclose(tmp);
    return rc;
}

int test_msh_repl_with_stdin(const char *input, bool interactive)
{
    msh_context_t ctx;
    FILE *tmp;
    int saved_stdin;
    int rc;

    saved_stdin = dup(STDIN_FILENO);
    TEST_ASSERT_TRUE(saved_stdin >= 0);
    tmp = tmpfile();
    TEST_ASSERT_TRUE(tmp != NULL);
    if (input != NULL)
    {
        TEST_ASSERT_EQ_INT(fwrite(input, 1U, strlen(input), tmp),
                           strlen(input));
    }
    rewind(tmp);
    TEST_ASSERT_EQ_INT(dup2(fileno(tmp), STDIN_FILENO), STDIN_FILENO);
    clearerr(stdin);

    memset(&ctx, 0, sizeof(ctx));
    ctx.interactive = interactive;
    rc = msh_repl(&ctx);

    TEST_ASSERT_EQ_INT(dup2(saved_stdin, STDIN_FILENO), STDIN_FILENO);
    clearerr(stdin);
    close(saved_stdin);
    fclose(tmp);
    return rc;
}
