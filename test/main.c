
#include "easy-yaml.h"
#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
    puts("\n\tPARSER\n");
    struct eyaml* root = NULL;
    int err = eyaml_parse(&root, stdin);
    if(err) {
        fprintf(stderr, "parser error %d\n", err);
        return 1;
    }
    assert(root);
    assert(YAML_SEQUENCE == eyaml_type(root));

    puts("\n\tDEBUG\n");
    eyaml_debug(root);
    puts("\n");

    printf("root len: %d\n", eyaml_length(root));

    struct eyaml* doc  = eyaml_index2child(root, 0);
    assert(doc);
    assert(YAML_MAPPING == eyaml_type(doc));

    printf("doc len: %d\n", eyaml_length(doc));


    struct eyaml* name = eyaml_name2child(doc, "name");
    assert(name);
    assert(YAML_SCALAR == eyaml_type(name));

    char const* nameval = eyaml_name2value(doc, "name");
    assert(nameval);
    printf("name: %s\n", nameval);

    struct eyaml* data = eyaml_name2child(doc, "data");
    assert(data);
    assert(YAML_MAPPING == eyaml_type(data));

    struct {
        char const* age;
        char const* color;
    } info;
    int num = eyaml_names2values(data, &info, "age", "color", NULL);
    assert(2==num);
    printf("age: %s\ncolor: %s\n", info.age, info.color);

    struct eyaml* age = eyaml_name2child(data, "age");
    assert(age);
    assert(YAML_SCALAR == eyaml_type(age));

    struct eyaml* color = eyaml_name2child(data, "color");
    assert(color);
    assert(YAML_SCALAR == eyaml_type(color));

    struct eyaml* scores = eyaml_name2child(doc, "scores");
    assert(scores);
    assert(YAML_SEQUENCE == eyaml_type(scores));
    assert(3 == eyaml_length(scores));

    puts("\n\tEMIT\n");
    eyaml_emit(root, stdout);

    eyaml_destroy(root);
    return 0;
}
