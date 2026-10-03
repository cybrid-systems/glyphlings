AURA_BIN ?= /home/dev/code/grok-dev/aura-grok/build/aura
AURA_PATH ?= /home/dev/code/grok-dev/aura-grok/lib
export AURA_BIN
export AURA_PATH
export AURA_SANDBOX = off
export AURA_PIPELINE_STRICT ?= force-soa

.PHONY: doctor test run line

doctor:
	@test -x "$(AURA_BIN)" || { echo "找不到 Aura 二进制：$(AURA_BIN)"; exit 127; }
	@echo "aura ok: $(AURA_BIN)"

test: doctor
	@sh tests/headless.sh

display/glyphlings-tty: display/glyphlings-tty.c
	cc -std=c11 -Wall -Wextra -Werror -o $@ display/glyphlings-tty.c

run: display/glyphlings-tty
	mkdir -p runtime
	@export LLM_MODEL="$${LLM_MODEL:-deepseek-flash}"; \
	export LLM_BASE_URL="$${LLM_BASE_URL:-https://api.deepseek.com}"; \
	export GLYPHLINGS_COACH="$${GLYPHLINGS_COACH:-1}"; \
	if [ -z "$${LLM_API_KEY:-}" ] && [ -f /home/dev/code/keys/deepseek ]; then \
	  export LLM_API_KEY="$$(tr -d ' \r\n' < /home/dev/code/keys/deepseek)"; \
	fi; \
	exec ./display/glyphlings-tty

line: doctor
	mkdir -p runtime
	@export LLM_MODEL="$${LLM_MODEL:-deepseek-flash}"; \
	export LLM_BASE_URL="$${LLM_BASE_URL:-https://api.deepseek.com}"; \
	export GLYPHLINGS_COACH="$${GLYPHLINGS_COACH:-1}"; \
	if [ -z "$${LLM_API_KEY:-}" ] && [ -f /home/dev/code/keys/deepseek ]; then \
	  export LLM_API_KEY="$$(tr -d ' \r\n' < /home/dev/code/keys/deepseek)"; \
	fi; \
	exec "$(AURA_BIN)" aura/main.aura
