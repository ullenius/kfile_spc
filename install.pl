#!/usr/bin/perl
# Helper script to install files.  Does not support DESTDIR, that must be done
# in the Makefile.

use File::Copy;

$path_sep = '/';
$install_dir = pop @ARGV;
@install_files = @ARGV;

@path_elements = split(/$path_sep/, $install_dir);
$element_path .= shift @path_elements;

# Skip if we're not installing.
exit 0 unless scalar @install_files;

while (1)
{
    if ($element_path and not -e $element_path)
    {
	mkdir ($element_path) or do {
	    print "Unable to create $element_path: $!\n";
	    exit 1;
	};
    }

    last unless scalar @path_elements;

    $next_element = shift @path_elements;
    $element_path .= $path_sep . $next_element;
}

# Now install.
for $file (@install_files)
{
    print "Installing $file -> $install_dir$path_sep$file\n";

    if (! -r $file)
    {
	print "$file doesn't exist (or is unreadable)\n";
	exit 1;
    }

    File::Copy::syscopy ($file, $install_dir) or do
    {
	print "Unable to install $file: $!\n";
	exit 1;
    };
}

exit 0;
