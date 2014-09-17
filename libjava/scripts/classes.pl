# classes.pl - A perl program to generate most of the contents of
# javaprims.h automatically.

# Copyright (C) 1998, 1999, 2000  Red Hat, Inc.
#
# This file is part of libjava.
#
# This software is copyrighted work licensed under the terms of the
# Libjava License.  Please consult the file "LIBJAVA_LICENSE" for
# details.

# Usage: cd <top-srcdir> ; perl classes.pl.
# Can also be run from the `include' directory; this lets us
# more easily insert the output into javaprims.h (which is where it goes).

use DirHandle;

if (-d 'java')
{
    # Ok here.
}
elsif (-d '../java')
{
    chdir ('..');
}
else
{
    die "couldn't find java directory\n";
}

&scan ('java', 2);

exit 0;

sub scan
{
    local ($dir, $indent) = @_;
    local (@subdirs) = ();
    local (%classes) = ();

    local ($d) = new DirHandle $dir;
    local (*JFILE);
    local ($name);
    if (defined $d)
    {
	while (defined ($name = $d->read))
	{
	    next if $name eq 'CVS';
	    next if $name eq '.';
	    next if $name eq '..';
	    if ($dir eq 'java'
		&& $name ne 'lang'
		&& $name ne 'util'
		&& $name ne 'io')
	    {
		# We only generate decls for java.lang, java.io, and
		# java.util.
		next;
	    }
	    if (-d ($dir . '/' . $name))
	    {
		push (@subdirs, $name);
		next;
	    }
	    next unless $name =~ /\.java$/;

	    open (FILE, "< $dir/$name");
	    local ($outer, $classname);
	    while (<FILE>)
	    {
		s,//.*$,,;
		# NOTE: we don't skip `/*' comments.  However, we do
		# skip lines with a `*' with leading whitespace.  This
		# catches the most important cases.
		s,^\s*\*.*$,,;

		# For now assume that class names start with upper
		# case letter.
		next unless /\b(class|interface) ([A-Z][A-Za-z0-9]+)/;
		$classname = $2;

		# We assume the code is properly indented, so that we
		# can print inner classes properly.
		if (/^\s/)
		{
		    die "no outer class for $classname in $dir/$name"
			unless $outer;
		    $classes{$outer . "\$" . $classname} = 1;
		}
		else
		{
		    $classes{$classname} = 1;
		    $outer = $classname;
		}
	    }
	    close (FILE);
	}

	undef $d;
    }

    local ($spaces) = ' ' x $indent;
    local ($classname);
    ($classname = $dir) =~ s/^.*\///;
    print $spaces, "namespace ", $classname, "\n";
    print $spaces, "{\n";

    foreach (sort keys %classes)
    {
	print $spaces, "  class ", $_, ";\n";
    }
    print "\n" if scalar @classes > 0 && scalar @subdirs > 0;

    local ($first) = 1;
    foreach (sort @subdirs)
    {
	print "\n" unless $first;
	$first = 0;
	&scan ("$dir/$_", $indent + 2);
    }

    print $spaces, "};\n";
}
