#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

LOG_MODULE_REGISTER(main);

/* Buffer (static array) for question-answer pairs */
static const struct {
	const char *question;
	const char *answer;
} response_buffer[] = {
	{"hello", "hii there"},
	{"bye", "see you later"},
	{"how are you", "I'm fine, thanks!"},
	/* Add more pairs here as needed */
};

/* Shell command handler: chat <message> */
static int cmd_chat(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: chat <message>");
		return -EINVAL;
	}

	char *message = argv[1];
	bool found = false;
	const char *response = NULL;

	/* Search the buffer for a matching question (case-insensitive) */
	for (size_t i = 0; i < ARRAY_SIZE(response_buffer); i++) {
		if (strcasecmp(message, response_buffer[i].question) == 0) {
			response = response_buffer[i].answer;
			found = true;
			break;
		}
	}

	if (found) {
		shell_print(sh, "%s", response);
	} else {
		shell_print(sh, "Sorry, no response found for '%s'.", message);
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(chat, NULL, "Chat with the bot: chat <message>", cmd_chat, 2, 0);

void main(void)
{
	LOG_INF("Chatbot ready. Responses stored in buffer. Use 'chat <message>' in shell.");
	/* Shell runs in its own thread; main can sleep */
	while (1) {
		k_sleep(K_MSEC(1000));
	}
}