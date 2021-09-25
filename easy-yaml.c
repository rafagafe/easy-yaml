

#include "easy-yaml.h"
#include <yaml.h>
#include <string.h>
#include <stdarg.h>

#define arraylen(arr) (sizeof (arr) / sizeof *(arr))

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* Hold a linked stack of pointers */
struct stack {
    struct item* top;
};

/* Node for the linked stack of pointers */
struct item {
    struct item* down; /* Points to the node below */
    void* data;        /* The pointer value to be stored */
};

/* Initialize a stack */
static void stack_init(struct stack* self) {
    self->top = NULL;
}

/* Check if the stack is empty */
static int stack_isempty(struct stack const* self) {
    return NULL == self->top;
}

/* Push a pointer on the stack */
static void stack_push(struct stack* self, void* data) {
    struct item* item = malloc(sizeof *item);
    item->data = data;
    item->down = self->top;
    self->top  = item;
}

/* Pop a pointer, return null on empty */
static void* stack_pop(struct stack* self) {
    if (stack_isempty(self))
        return NULL;
    struct item* item = self->top;
    self->top = self->top->down;
    void* data = item->data;
    free(item);
    return data;
}

/* Get the top pointer without remove it */
void* stack_pick(struct stack const* self) {
    if (stack_isempty(self))
        return NULL;
    return self->top->data;
}

/* Remove al pointers */
void stack_flush(struct stack* self) {
    while(!stack_isempty(self))
        stack_pop(self);
}

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */

/* Holds an easy-yaml object */
struct eyaml {
    struct eyaml* sibling;
    struct eyaml* child;
    yaml_event_t events[3];
};

/* Create an empty easy-yaml node */
static struct eyaml* eyaml_create(void) {
    struct eyaml* node = malloc(sizeof *node);
    if (NULL == node)
        return NULL;
    memset(node, 0, sizeof *node);
    return node;
}

/* Free a tree of easy-yaml nodes */
void eyaml_destroy(struct eyaml* self)  {
    if (NULL == self)
        return;
    struct stack garbage;
    stack_init(&garbage);
    stack_push(&garbage, self);
    do {
        struct eyaml* node = stack_pop(&garbage);
        if (NULL != node->sibling)
            stack_push(&garbage, node->sibling);
        if (NULL != node->child)
            stack_push(&garbage, node->child);
        for(int i = 0; i < arraylen(self->events); ++i)
            yaml_event_delete(self->events + i);
        free(node);
    } while(!stack_isempty(&garbage));
}

static void eyaml_appned(struct eyaml* self, struct eyaml* child) {
    child->child = NULL;
    if ( self->child == NULL )
        self->child = child;
    else {
        struct eyaml* iter;
        for( iter = self->child; NULL != iter->sibling; iter = iter->sibling );
        iter->sibling = child;
    }
}

static int istype(struct eyaml const* node, yaml_event_type_t a, yaml_event_type_t b, yaml_event_type_t c ) {
    return a == node->events[0].type && b == node->events[1].type && c == node->events[2].type;
}

/* Get the type of a node */
enum eyamltype eyaml_type(struct eyaml* self) {

    int isdoc = istype(self, YAML_DOCUMENT_START_EVENT, YAML_DOCUMENT_END_EVENT, YAML_NO_EVENT);
    if (isdoc)
        self = self->child;

    if ( istype(self, YAML_SCALAR_EVENT, YAML_SCALAR_EVENT, YAML_NO_EVENT) ||
         istype(self, YAML_SCALAR_EVENT, YAML_NO_EVENT, YAML_NO_EVENT) )
        return EYAML_SCALAR;

    if ( istype(self, YAML_SCALAR_EVENT, YAML_MAPPING_START_EVENT, YAML_MAPPING_END_EVENT) ||
         istype(self, YAML_MAPPING_START_EVENT, YAML_MAPPING_END_EVENT, YAML_NO_EVENT) )
        return EYAML_MAPPING;

    return EYAML_SEQUENCE;
}

/* Search in a mapping member node by its name */
struct eyaml* eyaml_name2child(struct eyaml* self, char const* name) {
    int isdoc = istype(self, YAML_DOCUMENT_START_EVENT, YAML_DOCUMENT_END_EVENT, YAML_NO_EVENT);
    if (isdoc)
        self = self->child;
    if ( EYAML_MAPPING != eyaml_type(self) )
        return NULL;
    for(struct eyaml* i = self->child; NULL != i; i = i->sibling)
        if (0 == strcmp(name, (char*)i->events[0].data.scalar.value))
            return i;
    return NULL;
}

/* Search in a mapping or sequence member node by its index */
struct eyaml* eyaml_index2child(struct eyaml* self, int index) {
    if ( istype(self, YAML_SCALAR_EVENT, YAML_SCALAR_EVENT, YAML_NO_EVENT) ||
         istype(self, YAML_SCALAR_EVENT, YAML_NO_EVENT, YAML_NO_EVENT) )
        return NULL;
    int isdoc = istype(self, YAML_DOCUMENT_START_EVENT, YAML_DOCUMENT_END_EVENT, YAML_NO_EVENT);
    if (isdoc)
        self = self->child;
    struct eyaml* child = self->child;
    for(int i = 0; i < index; ++i)
        child = child->sibling;
    if (NULL == child)
        return NULL;
    isdoc = istype(child, YAML_DOCUMENT_START_EVENT, YAML_DOCUMENT_END_EVENT, YAML_NO_EVENT);
    if (isdoc)
        child = self->child;
    return child;
}

/* Get the next sibling of a easy-yaml node */
struct eyaml* eyaml_sibling(struct eyaml* self) {
    return self->sibling;
}

/* Get the length of a easy-yaml node */
int eyaml_length(struct eyaml* self) {
    if (istype(self, YAML_SCALAR_EVENT, YAML_SCALAR_EVENT, YAML_NO_EVENT))
        return self->events[1].data.scalar.length;
    if (istype(self, YAML_SCALAR_EVENT, YAML_NO_EVENT, YAML_NO_EVENT))
        return self->events[0].data.scalar.length;
    int isdoc = istype(self, YAML_DOCUMENT_START_EVENT, YAML_DOCUMENT_END_EVENT, YAML_NO_EVENT);
    if (isdoc)
        self = self->child;
    int cnt = 0;
    for(struct eyaml const* i = self->child; i; i = i->sibling)
        ++cnt;
    return cnt;
}

/* Get the length of the name of a mapping child */
int eyaml_namelen(struct eyaml * self) {
    if (YAML_SCALAR_EVENT != self->events[0].type || YAML_NO_EVENT == self->events[1].type)
        return 0;
    return self->events[0].data.scalar.length;
}

/* Get the name of a mapping child node */
char const* eyaml_name(struct eyaml* self) {
    if (YAML_SCALAR_EVENT != self->events[0].type || YAML_NO_EVENT == self->events[1].type)
        return NULL;
    return (char*)self->events[0].data.scalar.value;
}

/* Get the value of a scalar node */
char const* eyaml_value(struct eyaml* self) {
    int isdoc = istype(self, YAML_DOCUMENT_START_EVENT, YAML_DOCUMENT_END_EVENT, YAML_NO_EVENT);
    if (isdoc)
        self = self->child;
    if (EYAML_SCALAR != eyaml_type(self))
        return NULL;
    int index = YAML_SCALAR_EVENT == self->events[1].type ? 1 : 0;
    return (char*)self->events[index].data.scalar.value;
}

/* Get multiple field values from a mapping node */
int eyaml_names2values(struct eyaml* self, void* dest, ...) {
    if(NULL == self || NULL == dest)
        return 0;
    va_list valist;
    va_start(valist, dest);
    char const* (*ptr)[] = (char const*(*)[])dest;
    char const* name;
    int i = 0;
    while(( name = va_arg(valist, char*) ))
        (*ptr)[i++] = eyaml_name2value(self, name);
    return i;
}

/* Get multiple field values from a mapping node */
int eyaml_values(struct eyaml* self, void* dest, char const* names[] ) {
    int cnt = 0;
    char const* (*values)[] = (char const*(*)[])dest;
    for(int i = 0; NULL != names[i]; ++i) {
        (*values)[i] = eyaml_name2value(self, names[i]);
        if(NULL != (*values)[i])
            ++cnt;
    }
    return cnt;
}


static int isclosing(yaml_event_type_t event) {
    return
        YAML_STREAM_END_EVENT    == event ||
        YAML_DOCUMENT_END_EVENT  == event ||
        YAML_SEQUENCE_END_EVENT  == event ||
        YAML_MAPPING_END_EVENT   == event;
}

static int emitNotClosingEvents(struct eyaml* self, yaml_emitter_t* emitter) {
    for(int i = 0; i < arraylen(self->events); ++i) {
        yaml_event_type_t type = self->events[i].type;
        if (YAML_NO_EVENT == type)
            return 0;
        if (!isclosing(type))
            if (!yaml_emitter_emit(emitter, self->events + i))
                return -1;
    }
    return 0;
}

static int emitClosingEvents(struct eyaml* self, yaml_emitter_t* emitter) {
    for(int i = 0; i < arraylen(self->events); ++i) {
        yaml_event_type_t type = self->events[i].type;
        if (YAML_NO_EVENT == type)
            return 0;
        if (isclosing(type))
            if (!yaml_emitter_emit(emitter, self->events + i))
                return -1;
    }
    return 0;
}

/* Dump a tree of easy-yaml nodes to a stream */
int eyaml_emit(struct eyaml* self, FILE* strm) {

    if (NULL == self)
        return 0;

    yaml_emitter_t emitter;
    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output_file(&emitter, strm);

    struct stack nodes; /* nodes to be emitted */
    stack_init(&nodes);
    stack_push(&nodes, self);

    do {

        int shouldclose = 0;
        struct eyaml* node = stack_pick(&nodes);

        if (eyaml_type(node) == EYAML_SCALAR) {
            stack_pop(&nodes);
            struct eyaml* sibling = eyaml_sibling(node);
            if (NULL != sibling)
                stack_push(&nodes, sibling);
            else
                shouldclose = 1;
        }
        else {
            struct eyaml* child = node->child;
            if (NULL != child)
                 stack_push(&nodes, child);
            else
                shouldclose = 1;
        }

        int err = emitNotClosingEvents(node, &emitter);
        if (err)
            goto error;

        if (shouldclose) for(;;) {
            struct eyaml* parent = stack_pop(&nodes);
            if ( NULL == parent )
                break;
            int err = emitClosingEvents(parent, &emitter);
            if (err)
                goto error;
            struct eyaml* sibling = eyaml_sibling(parent) ;
            if (NULL != sibling) {
                stack_push(&nodes, sibling);
                break;
            }
        }

    } while(!stack_isempty(&nodes));


    /* success: */
    return 0;

  error:
    stack_flush(&nodes);
    return -1;
}



static void printevent(yaml_event_t *event, int* level);

static void printNotClosingEvents(struct eyaml* self, int* level) {
    for(int i = 0; i < arraylen(self->events); ++i) {
        yaml_event_type_t type = self->events[i].type;
        if (YAML_NO_EVENT == type)
            break;
        if (!isclosing(type))
            printevent(self->events + i, level);
    }
}

static void printClosingEvents(struct eyaml* self, int* level) {
    for(int i = 0; i < arraylen(self->events); ++i) {
        yaml_event_type_t type = self->events[i].type;
        if (YAML_NO_EVENT == type)
            break;
        if (isclosing(type))
            printevent(self->events + i, level);
    }
}

/* Print in stdout debug info */
void eyaml_debug(struct eyaml* self) {


    if (NULL == self)
        return;

    struct stack nodes; /* nodes to be printed */
    stack_init(&nodes);
    stack_push(&nodes, self);

    int level = 0;

    do {

        int shouldclose = 0;
        struct eyaml* node = stack_pick(&nodes);

        if (eyaml_type(node) == EYAML_SCALAR) {
            stack_pop(&nodes);
            struct eyaml* sibling = eyaml_sibling(node);
            if (NULL != sibling)
                stack_push(&nodes, sibling);
            else
                shouldclose = 1;
        }
        else {
            struct eyaml* child = node->child;
            if (NULL != child)
                 stack_push(&nodes, child);
            else
                shouldclose = 1;
        }

        printNotClosingEvents(node, &level);

        if (shouldclose) for(;;) {
            struct eyaml* parent = stack_pop(&nodes);
            if ( NULL == parent )
                break;
            printClosingEvents(parent, &level);
            struct eyaml* sibling = eyaml_sibling(parent) ;
            if (NULL != sibling) {
                stack_push(&nodes, sibling);
                break;
            }
        }

    } while(!stack_isempty(&nodes));

}

/* Parse a YAML stream */
int eyaml_parse(struct eyaml** dest, FILE* src) {

    struct stack wip; // the stack to store Work In Progress yaml nodes
    stack_init(&wip);

    yaml_parser_t parser;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, src);

    int level = 0;
    int err = -1;
    *dest = NULL;

    do {

        yaml_event_t event;
        err = !yaml_parser_parse(&parser, &event);
        if (err) {
            fprintf(stderr, "yaml_parser_parse error\n");
            goto done;
        }

        if (0)
            printevent(&event, &level);

        switch(event.type) {

            case YAML_STREAM_START_EVENT: {
                if (!stack_isempty(&wip)) {
                    err = -1;
                    goto done;
                }
                struct eyaml* node = eyaml_create();
                node->events[0] = event;
                stack_push(&wip, node);
                break;
            }

            case YAML_STREAM_END_EVENT: {
                struct eyaml* stream = stack_pick(&wip);
                if ( NULL == stream || !istype( stream, YAML_STREAM_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT ) ) {
                    err = -2;
                    goto done;
                }
                stack_pop(&wip);
                stream->events[1] = event;
                *dest = stream;
                break;
            }

            case YAML_DOCUMENT_START_EVENT: {
                struct eyaml* stream = stack_pick(&wip);
                if (NULL == stream || !istype(stream, YAML_STREAM_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    err = -3;
                    goto done;
                }
                struct eyaml* doc = eyaml_create();
                doc->events[0] = event;
                eyaml_appned(stream, doc);
                stack_push(&wip, doc);
                break;
            }

            case YAML_DOCUMENT_END_EVENT: {
                struct eyaml* doc = stack_pick(&wip);
                if (NULL == doc || !istype(doc, YAML_DOCUMENT_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    err = -4;
                    goto done;
                }
                stack_pop(&wip);
                doc->events[1] = event;
                break;
            }

            case YAML_MAPPING_START_EVENT: {
                struct eyaml* node = stack_pick(&wip);
                if (NULL == node) {
                    err = -5;
                    goto done;
                }
                int const isdoc = istype(node, YAML_DOCUMENT_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT);
                if (isdoc && eyaml_haschildren(node)) {
                    err = -6;
                    goto done;
                }
                if (
                    isdoc ||
                    istype(node, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT) ||
                    istype(node, YAML_SCALAR_EVENT, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT)
                ) {
                    struct eyaml* map = eyaml_create();
                    map->events[0] = event;
                    eyaml_appned(node, map);
                    stack_push(&wip, map);
                    break;
                }
                else if (istype(node, YAML_SCALAR_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    node->events[1] = event;
                }
                else {
                    err = -7;
                    goto done;
                }
                break;
            }

            case YAML_MAPPING_END_EVENT: {
                struct eyaml* map = stack_pick(&wip);
                if (NULL == map) {
                    err = -8;
                    goto done;
                }
                if (istype(map, YAML_MAPPING_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    stack_pop(&wip);
                    map->events[1] = event;
                }
                else if (istype(map, YAML_SCALAR_EVENT, YAML_MAPPING_START_EVENT, YAML_NO_EVENT)) {
                    stack_pop(&wip);
                    map->events[2] = event;
                }
                else {
                    err = -9;
                    goto done;
                }
                break;
            }

            case YAML_SEQUENCE_START_EVENT: {
                struct eyaml* node = stack_pick(&wip);
                if (NULL == node) {
                    err = -10;
                    goto done;
                }
                int const isdoc = istype(node, YAML_DOCUMENT_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT);
                if (isdoc && eyaml_haschildren(node)) {
                    err = -11;
                    goto done;
                }
                if (
                    isdoc ||
                    istype(node, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT) ||
                    istype(node, YAML_SCALAR_EVENT, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT)
                ) {
                    struct eyaml* map = eyaml_create();
                    map->events[0] = event;
                    eyaml_appned(node, map);
                    stack_push(&wip, map);
                    break;
                }
                else if (istype(node, YAML_SCALAR_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    node->events[1] = event;
                }
                else {
                    err = -12;
                    goto done;
                }
                break;
            }

            case YAML_SEQUENCE_END_EVENT: {
                struct eyaml* seq = stack_pick(&wip);
                if (NULL == seq) {
                    err = -13;
                    goto done;
                }
                if (istype(seq, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    stack_pop(&wip);
                    seq->events[1] = event;
                }
                else if (istype(seq, YAML_SCALAR_EVENT, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT)) {
                    stack_pop(&wip);
                    seq->events[2] = event;
                }
                else {
                    err = -14;
                    goto done;
                }
                break;
            }

            case YAML_SCALAR_EVENT: {
                struct eyaml* node = stack_pick(&wip);
                if (
                    istype(node, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT) ||
                    istype(node, YAML_SCALAR_EVENT, YAML_SEQUENCE_START_EVENT, YAML_NO_EVENT)
                ) {
                    struct eyaml* scalar = eyaml_create();
                    scalar->events[0] = event;
                    eyaml_appned(node, scalar);
                }
                else if (
                    istype(node, YAML_MAPPING_START_EVENT, YAML_NO_EVENT, YAML_NO_EVENT) ||
                    istype(node, YAML_SCALAR_EVENT, YAML_MAPPING_START_EVENT, YAML_NO_EVENT)
                ) {
                    struct eyaml* scalar = eyaml_create();
                    scalar->events[0] = event;
                    eyaml_appned(node, scalar);
                    stack_push(&wip, scalar);
                }
                else if (istype(node, YAML_SCALAR_EVENT, YAML_NO_EVENT, YAML_NO_EVENT)) {
                    stack_pop(&wip);
                    node->events[1] = event;
                }
                else {
                    err = -14;
                    goto done;
                }
                break;
            }

            case YAML_NO_EVENT:
                break;

            default:
                err = -15;
                goto done;
        }

    } while (NULL == *dest);

    err = 0;

  done:
    if ( !stack_isempty(&wip) ) {
        struct eyaml* root = NULL;
        do root = stack_pop(&wip);
            while(!stack_isempty(&wip));
        eyaml_destroy(root);
        if ( 0 == err )
            err = -20;
    }



    return err;
}


#define INDENT "  "
#define STRVAL(x) ((x) ? (char*)(x) : "")

void indent(int level)
{
    int i;
    for (i = 0; i < level; i++) {
        printf("%s", INDENT);
    }
}

static void printevent(yaml_event_t *event, int* level) {

    switch (event->type) {
    case YAML_NO_EVENT:
        //indent(*level);
        //printf("no-event (%d)\n", event->type);
        break;
    case YAML_STREAM_START_EVENT:
        indent((*level)++);
        printf("stream-start-event (%d)\n", event->type);
        break;
    case YAML_STREAM_END_EVENT:
        indent(--(*level));
        printf("stream-end-event (%d)\n", event->type);
        break;
    case YAML_DOCUMENT_START_EVENT:
        indent((*level)++);
        printf("document-start-event (%d)\n", event->type);
        break;
    case YAML_DOCUMENT_END_EVENT:
        indent(--(*level));
        printf("document-end-event (%d)\n", event->type);
        break;
    case YAML_ALIAS_EVENT:
        indent(*level);
        printf("alias-event (%d)\n", event->type);
        break;
    case YAML_SCALAR_EVENT:
        indent(*level);
        printf("scalar-event (%d) = {value=\"%s\", length=%d}\n",
               event->type,
               STRVAL(event->data.scalar.value),
               (int)event->data.scalar.length);
        break;
    case YAML_SEQUENCE_START_EVENT:
        indent((*level)++);
        printf("sequence-start-event (%d)\n", event->type);
        break;
    case YAML_SEQUENCE_END_EVENT:
        indent(--(*level));
        printf("sequence-end-event (%d)\n", event->type);
        break;
    case YAML_MAPPING_START_EVENT:
        indent((*level)++);
        printf("mapping-start-event (%d)\n", event->type);
        break;
    case YAML_MAPPING_END_EVENT:
        indent(--(*level));
        printf("mapping-end-event (%d)\n", event->type);
        break;
    }
    if (*level < 0) {
        fprintf(stderr, "indentation underflow!\n");
        *level = 0;
    }
}
