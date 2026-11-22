#include <libxml/parser.h>
#include <libxml/tree.h>

#include "vm_internal.h"

static struct vm_arg_type vm_parse_type(const char *type_str)
{
    struct vm_arg_type t;
    size_t len;

    t.base     = VM_BASE_U8;
    t.is_array = 0;
    if (!type_str)
        return (t);

    len = strlen(type_str);
    if (len > 2 && type_str[len - 2] == '[' && type_str[len - 1] == ']') {
        t.is_array = 1;
        len -= 2;
    }

    unsigned int bits = 0;
    size_t       j;

    if (strstr(type_str, "str") != NULL) { t.base = VM_BASE_STR; return (t); }

    for (j = 0; j < len; j++) {
        if (type_str[j] >= '0' && type_str[j] <= '9')
            bits = bits * 10 + (unsigned int)(type_str[j] - '0');
    }
    switch (bits / 8) {
        case 1: t.base = VM_BASE_U8;  return (t);
        case 2: t.base = VM_BASE_U16; return (t);
        case 4: t.base = VM_BASE_U32; return (t);
        case 8: t.base = VM_BASE_U64; return (t);
    }
    return (t);
}

static int vm_parse_op_arg(struct vm_op_def *def, xmlNodePtr arg_node)
{
    struct vm_arg_def *new_args;
    xmlChar *type_attr;
    xmlChar *label_attr;

    if (!def || !arg_node)
        return (-1);

    type_attr  = xmlGetProp(arg_node, (const xmlChar *)"type");
    label_attr = xmlGetProp(arg_node, (const xmlChar *)"label");
    if (!type_attr || !label_attr) {
        xmlFree(type_attr);
        xmlFree(label_attr);
        return (-1);
    }

    new_args = (struct vm_arg_def *)realloc(def->args,
                   (def->arg_count + 1) * sizeof(struct vm_arg_def));
    if (!new_args) {
        xmlFree(type_attr);
        xmlFree(label_attr);
        return (-1);
    }
    def->args = new_args;
    def->args[def->arg_count].type  = vm_parse_type((const char *)type_attr);
    def->args[def->arg_count].label = strdup((const char *)label_attr);
    xmlFree(type_attr);
    xmlFree(label_attr);

    if (!def->args[def->arg_count].label)
        return (-1);
    def->arg_count++;
    return (0);
}

static int vm_parse_op(struct vm_iset_s *iset, xmlNodePtr op_node)
{
    struct vm_op_def *new_ops;
    struct vm_op_def *def;
    xmlNodePtr arg_node;
    xmlChar *name_attr;
    xmlChar *code_attr;

    if (!iset || !op_node)
        return (-1);

    name_attr = xmlGetProp(op_node, (const xmlChar *)"name");
    code_attr = xmlGetProp(op_node, (const xmlChar *)"code");
    if (!name_attr || !code_attr) {
        xmlFree(name_attr);
        xmlFree(code_attr);
        return (-1);
    }

    new_ops = (struct vm_op_def *)realloc(iset->ops,
                  (iset->op_count + 1) * sizeof(struct vm_op_def));
    if (!new_ops) {
        xmlFree(name_attr);
        xmlFree(code_attr);
        return (-1);
    }
    iset->ops = new_ops;
    def = &iset->ops[iset->op_count];
    def->name      = strdup((const char *)name_attr);
    def->code      = (uint16_t)atoi((const char *)code_attr);
    def->arg_count = 0;
    def->args      = NULL;
    xmlFree(name_attr);
    xmlFree(code_attr);

    if (!def->name)
        return (-1);

    for (arg_node = op_node->children; arg_node; arg_node = arg_node->next) {
        if (arg_node->type != XML_ELEMENT_NODE)
            continue;
        if (xmlStrcmp(arg_node->name, (const xmlChar *)"arg") != 0)
            continue;
        vm_parse_op_arg(def, arg_node);
    }

    iset->op_count++;
    return (0);
}

int vm_parse_xml(const char *path, struct vm_iset_s *iset)
{
    xmlDocPtr doc;
    xmlNodePtr root;
    xmlNodePtr op_node;
    xmlNodePtr child;
    xmlChar *attr;

    if (!path || !iset)
        return (-1);

    doc = xmlReadFile(path, NULL, 0);
    if (!doc)
        return (-1);

    root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        return (-1);
    }

    if (xmlStrcmp(root->name, (const xmlChar *)"wizard") == 0) {
        for (child = root->children; child; child = child->next) {
            if (child->type == XML_ELEMENT_NODE &&
                xmlStrcmp(child->name, (const xmlChar *)"instructions") == 0) {
                root = child;
                break;
            }
        }
    }

    if (xmlStrcmp(root->name, (const xmlChar *)"instructions") != 0) {
        xmlFreeDoc(doc);
        return (-1);
    }

    attr = xmlGetProp(root, (const xmlChar *)"version");
    if (!attr) {
        xmlFreeDoc(doc);
        return (-1);
    }
    iset->version = (uint16_t)atoi((const char *)attr);
    xmlFree(attr);

    iset->op_count = 0;
    iset->ops      = NULL;

    for (op_node = root->children; op_node; op_node = op_node->next) {
        if (op_node->type != XML_ELEMENT_NODE)
            continue;
        if (xmlStrcmp(op_node->name, (const xmlChar *)"op") != 0)
            continue;
        if (vm_parse_op(iset, op_node) != 0)
            break;
    }

    xmlFreeDoc(doc);
    return (iset->op_count > 0 ? 0 : -1);
}
