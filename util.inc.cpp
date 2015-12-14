char *readfile(const char *filename) {
	char *source = NULL;
	size_t read;
	struct stat fi;
	FILE *fp = NULL;

	if (stat(filename, &fi) == -1) goto error;
	source = (char*)malloc(sizeof(char) * (fi.st_size + 1));
	if (source == NULL) goto error;
	fp = fopen(filename, "r");
	if (!fp) goto error;
	read = fread(source, sizeof(char), fi.st_size, fp);
	if (read != fi.st_size) goto error;
	source[fi.st_size] = '\0';
	goto ok;

error:
	if (source) {
		free(source);
		source = NULL;
	}

ok:
	if (fp) {
		fclose(fp);
	}

	return source;
}

int streql(const char *str1, const char *str2, int len2) {
	int p = 0;
    while (str1[p] && p < len2) {
        if (str1[p] != str2[p]) return 0;
        p++;
    }
    return str1[p] == 0 && p == len2;
}