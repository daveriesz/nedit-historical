/* SCCS ID: tags.h 1.2 9/1/94 */
int LoadTagsFile(char *filename);
int LookupTag(char *name, char **file, char **searchString);
void FindDefinition(WindowInfo *window, Time time);
int TagsFileLoaded();
