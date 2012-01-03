#include <gtk/gtk.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "chatgui.h"
#include "commons.h"

GtkWindow *window;
GtkTextView *chatView;
GtkEntry *chatEntry;

GIOChannel* chan;

int readfd, writefd;

int guibufferlen = 1;
char* guibuffer;

void handleGUISending ()
{
	write(writefd, gtk_entry_get_text(chatEntry), gtk_entry_get_text_length(chatEntry));
	write(writefd, "\n", 2);

	gtk_entry_set_text(chatEntry, "");
}

G_MODULE_EXPORT void buttonClicked (GtkButton *button)
{
	handleGUISending();
}

G_MODULE_EXPORT void entryActivated (GtkEntry *entry)
{
	handleGUISending();
}

void drawGUI ()
{
	GtkBuilder *builder;
	GError *error = NULL;

	// Init GTK+
	gtk_init(0, NULL);

	// Create new GtkBuilder object
	builder = gtk_builder_new();
	// Load UI from file. If error occurs, report it and quit application.
	if (!gtk_builder_add_from_file(builder, "glade/gui.glade", &error))
	{
		g_warning("%s", error->message);
		g_free(error);
		exit(1);
	}

	// Get main window pointer from UI
	window = GTK_WINDOW(gtk_builder_get_object(builder, "chatWindow"));

	// Get objects
	chatView = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "chatView"));
	chatEntry = GTK_ENTRY(gtk_builder_get_object(builder, "chatEntry"));

	// Connect signals
	gtk_builder_connect_signals(builder, NULL);

	// Destroy builder, since we don't need it anymore
	g_object_unref(G_OBJECT(builder));

	// Show window. All other widgets are automatically shown by GtkBuilder
	gtk_widget_show_all(GTK_WIDGET(window));
	
	gtk_window_set_focus(window, GTK_WIDGET(chatEntry));
	
	gtk_main();
}

void handleGUIMessage (char* msg)
{
	GtkTextIter iter;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(chatView);

	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, msg, -1);
}

void handleGUIDisconnect (int sock)
{
	g_io_channel_shutdown(chan, 0, NULL);

	close(writefd);
	close(readfd);

	free(guibuffer);
	exit(0);
}

gboolean handleChannel (GIOChannel *source, GIOCondition condition, gpointer data)
{
	if (condition == G_IO_HUP)
	{
		handleGUIDisconnect(g_io_channel_unix_get_fd(source));
	}
	else if (condition == G_IO_IN)
	{
		handleSocket(g_io_channel_unix_get_fd(source), &guibuffer, &guibufferlen, handleGUIMessage, handleGUIDisconnect);
	}

	return 1;
}

pid_t gui_start (int *infd, int *outfd)
{
	signal(SIGCLD, SIG_IGN);

	int pipefd_in[2];
	int pipefd_out[2];

	if (pipe(pipefd_in) == -1)
	{
		fprintf(stderr, "Pipe Error: %s\n", strerror(errno));
		exit(1);
	}

	if (pipe(pipefd_out) == -1)
	{
		fprintf(stderr, "Pipe Error: %s\n", strerror(errno));
		exit(1);
	}

	*infd = pipefd_in[0];
	*outfd = pipefd_out[1];

	pid_t childpid = fork();

	if (childpid < 0)
	{
		fprintf(stderr, "Fork Error: %s\n", strerror(errno));
		exit(1);
	}
	else if (childpid == 0)
	{
		// initialize buffer
		guibuffer = (char*) malloc(sizeof(char) * guibufferlen);
		if (guibuffer == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		guibuffer[0] = '\0';

		// close unused read and write ends
		close(pipefd_in[0]);
		close(pipefd_out[1]);

		readfd = pipefd_out[0];
		writefd = pipefd_in[1];

		chan = g_io_channel_unix_new(readfd);
		GIOCondition condition = G_IO_IN | G_IO_IN;
		g_io_add_watch(chan, condition, handleChannel, NULL);

		drawGUI();

		handleGUIDisconnect(0);
	}
	else
	{
		// close unused read and write ends
		close(pipefd_in[1]);
		close(pipefd_out[0]);

		return childpid;
	}

	return 0;
}
