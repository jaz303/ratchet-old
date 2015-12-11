char *readfile(const char *filename) {
	char *source = NULL;
	FILE *fp = fopen(filename, "r");
	if (!fp) goto error;
	if (fseek(fp, 0L, SEEK_END) != 0) goto error;
	long size = ftell(fp);
	if (size == -1) goto error;
	source = malloc(sizeof(char) * (size + 1));
	if (source == NULL) goto error;
	if (fseek(fp, 0L, SEEK_SET) != 0) goto error;
	size_t read = fread(source, sizeof(char), size, fp);
	if (read != size) goto error;
	source[read+1] = '\0';
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