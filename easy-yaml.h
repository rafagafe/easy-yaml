
#ifndef EASY_YAML_H
#define EASY_YAML_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/** @defgroup easyyaml Easy YAML.
  * This module parses YAML files and generates a tree of linked nodes.
  * Navigating or querying this tree is very easy using the provided API.
  * This module uses LIBYAML.
  * @{ */

/** Holds an easy-yaml object */
struct eyaml;

/** Type of data that the easy-yaml nodes can hold */
enum eyamltype {
    EYAML_SCALAR,
    EYAML_MAPPING,
    EYAML_SEQUENCE
};

/** Parse a YAML stream
  * @param [out] root Destination easy-yaml handle
  * @param [in]  src  Source stream
  * @return Zero on success, non-zero on error */
int eyaml_parse(struct eyaml** root, FILE* src);

/** Free a tree of easy-yaml nodes
  * @param root The root of the tree */
void eyaml_destroy(struct eyaml* root);

/** Search in a mapping member node by its name
  * @param [in] self The easy-yaml parent mapping node where to search
  * @param [in] name The name of the child node to find
  * @return The easy-yaml node on found, null pointer on other cases */
struct eyaml* eyaml_name2child(struct eyaml* self, char const* name);

/** Search in a mapping or sequence member node by its index
  * @param [in] self   The easy-yaml parent mapping or sequence node where to search
  * @param [in] index The index of the child node to find
  * @return The easy-yaml node on found, null pointer on other cases */
struct eyaml* eyaml_index2child(struct eyaml* self, int i);

/** Get the first child of a sequence or mapping node
  * @param [in] self   The easy-yaml parent mapping or sequence node where to search
  * @return If has any children the easy-yaml node else a null pointer */
static inline struct eyaml* eyaml_child(struct eyaml* self) {
    return eyaml_index2child(self, 0);
}

/** Check if a easy-yaml node has children or not
  * @param [in] self A valid handle of a easy-yaml node
  * @return Zero if it has not children else non-zero */
static inline int eyaml_haschildren(struct eyaml* parent) {
    return NULL != eyaml_child(parent);
}

/** Get the next sibling of a easy-yaml node
  * @param [in] self A valid handle of a easy-yaml node
  * @return If has any sibling the handle else null pointer */
struct eyaml* eyaml_sibling(struct eyaml* self);

/** Get the length of a easy-yaml node
  * If the node is a scalar then
  *   gets the length in chars of the value
  * else
  *   gets the number of childs
  * @param [in] self A valid handle of a easy-yaml node
  * @return the number of characters or children*/
int eyaml_length(struct eyaml* self);

/** Get the length of the name of a mapping child
  * @param [in] self A valid handle of a easy-yaml node
  * @return The number of character of the name */
int eyaml_namelen(struct eyaml* self);

/** Get the name of a mapping child node
  * @param [in] self A valid handle of a easy-yaml node
  * @return A null-terminated string with the name, null on not a mapping child */
char const* eyaml_name(struct eyaml* self);

/** Get the value of a scalar node
  * @param [in] self A valid handle of a easy-yaml node
  * @return A null-terminates string with the value, null on not a scalar node */
char const* eyaml_value(struct eyaml* self);

/** Search in a mapping a scalar member node by its name and get its value
  * @param [in] self A valid handle of a easy-yaml node
  * @param [in] name The name of the child node to find
  * @return A null-terminates string with the value on found a scalar member, null pointer on other cases */
static inline char const* eyaml_name2value(struct eyaml* self, char const* name) {
    return eyaml_value( eyaml_name2child(self, name) );
}

/** Search in a mapping or sequence a scalar member node by its index and get its value
  * @param [in] self  The easy-yaml parent mapping or sequence node where to search
  * @param [in] index The index of the child node to find
  * @return A null-terminates string with the value on found a scalar member, null pointer on other cases */
static inline char const* eyaml_index2value(struct eyaml* self, int i) {
    return eyaml_value( eyaml_index2child(self, i) );
}

/** Get multiple field values from a mapping node
  * @param [in]  self A valid handle of a easy-yaml node
  * @param [out] dest Destination array or structure of 'char const*'
  * @param arg list   Null-terminated list of names of the fields to be searched
  * @return Number of fields found */
int eyaml_names2values(struct eyaml* self, void* dest, ...);

/** Get multiple field values from a mapping node
  * @param [in]  self  A valid handle of a easy-yaml node
  * @param [out] dest  Destination array or structure of 'char const*'
  * @param [in]  names Null-terminated array of names of the fields to be searched
  * @return Number of fields found */
int eyaml_values(struct eyaml* self, void* dest, char const* names[]);

/** Get the type of a node
  * @param [in] self A valid handle of a easy-yaml node
  * @return The type code */
enum eyamltype eyaml_type(struct eyaml* self);

/** Dump a tree of easy-yaml nodes to a stream
  * @param [in]  self The root node of the tree
  * @param [out] dest Destination stream */
int eyaml_emit(struct eyaml* self, FILE* dest);

/** Print in stdout debug info */
void eyaml_debug(struct eyaml* self);

/** @ } */

#ifdef __cplusplus
}
#endif

#endif /* EASY_YAML_H */
