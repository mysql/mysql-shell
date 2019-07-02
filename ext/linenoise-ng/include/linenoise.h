/* linenoise.h -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * See linenoise.c for more information.
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Contains changes made for mysql shell

#ifndef __LINENOISE_H
#define __LINENOISE_H

#define LINENOISE_VERSION "1.0.0my"
#define LINENOISE_VERSION_MAJOR 1
#define LINENOISE_VERSION_MINOR 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct linenoiseCompletions linenoiseCompletions;

typedef void(linenoiseCompletionCallback)(const char*, int*,
                                          linenoiseCompletions*);
void linenoiseSetCompletionCallback(linenoiseCompletionCallback* fn);
void linenoiseAddCompletion(linenoiseCompletions* lc, const char* str);
char* linenoise(const char* prompt);
void linenoisePreloadBuffer(const char* preloadText);
int linenoiseHistoryAdd(const char* line, bool force = false);
int linenoiseHistoryDelete(int index);
int linenoiseHistorySetMaxLen(int len);
const char* linenoiseHistoryLine(int index);
int linenoiseHistorySize(void);
int linenoiseHistorySave(const char* filename);
int linenoiseHistoryLoad(const char* filename);
void linenoiseHistoryFree(void);
void linenoiseClearScreen(void);
void linenoiseSetMultiLine(int ml);
void linenoisePrintKeyCodes(void);
/* the following is extension to the original linenoise API */
int linenoiseInstallWindowChangeHandler(void);
int linenoiseKeyType(void);

int getWcwidth(char32_t ucs);

/**
 * Callback for a custom command.
 *
 * @param input - current contents of the input buffer
 * @param data - user data
 * @param output - if not NULL, new contents of the input buffer, linenoise
 *                 will release this memory with free()
 *
 * @returns next unicode character or -1 to read the character from console
 */
typedef int(linenoiseCustomCommand)(const char *input, void *data,
                                    char **output);
void linenoiseRegisterCustomCommand(const char *sequence,
                                    linenoiseCustomCommand *cmd, void *data);
void linenoiseRemoveCustomCommand(const char *sequence);

#ifdef __cplusplus
}
#endif

#endif /* __LINENOISE_H */
