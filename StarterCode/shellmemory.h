void mem_init();
void mem_init_OS_shell();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
void mem_set_script_in_memory(char *line, int line_number);
char *mem_get_value_OS_shell(int index);
void clean_script_from_memory(int start, int end);
void clean_single_line_in_memory(int i);
void mem_init_frame_Store();
void set_frame_memory(char *line, int index);
void printFrameStore();
char *mem_get_value_frame_store(int index);
int look_for_empty_frame();
void mem_init_variable_Store();