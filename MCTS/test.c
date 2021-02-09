#include <glib.h>
#include <glib/gprintf.h>

/*
 * Defines a macro for casting an object into a 'Person'
 */
#define PERSON(o) (Person*)(o)


typedef struct
{
        gchar * name;
        gint age;
} Person;


Person *
person_new (const gchar * name, gint age)
{
        Person * p;
        p = g_new (Person, 1);
        p->name = g_strdup (name);
        p->age = age;
        return p;
}


void
person_print (Person * p)
{
        g_printf ("Name: %s\nAge: %d\n\n", p->name, p->age);
}


void
person_free (Person * p)
{
        g_free (p->name);
        g_free (p);
}


gboolean
traverse_free_func (GNode * node, gpointer data)
{
        person_free (PERSON(node->data));
        return FALSE;
}


int
main (int argc, char *argv[])
{
        GNode * father;
        GNode * dother;
        GNode * brother;

        father = g_node_new (person_new("John", 50));
        dother = g_node_new (person_new("Sophia", 21));
        brother = g_node_new (person_new("Bob", 20));
        g_node_append (father, dother);
        g_node_insert_after (father, dother, brother);
        /* Fetch data */
        GNode * first_child;
        GNode * sibling;
        first_child = g_node_first_child (father);
        sibling = g_node_next_sibling (first_child);
        person_print (PERSON(father->data));
        person_print (PERSON(first_child->data));
        person_print (PERSON(sibling->data));

	(PERSON(father->data))->age = 100;

	person_print (PERSON(sibling->parent->data));
	person_print (PERSON(first_child->parent->data));
        /* We must free everything now. */
        g_node_traverse
        (
                father,
                G_IN_ORDER,
                G_TRAVERSE_ALL,
                -1,
                traverse_free_func,
                NULL
        );
        g_node_destroy (father);
        return 0;
}

/* EOF */
