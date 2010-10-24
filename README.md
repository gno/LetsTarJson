JSON tar. 

I've Been getting intimate with daemontools as of late. The part of
djb's mental model that's left the greatest impression on me is the
treatment of directories as key value stores. 

Also, I'm working on a json validator that keeps it's test suite on 
in files, as portable things like bourne shell handle files well.

json_to_tar.c reads a JSON value (which must be an object) and
spits out a tarball. 

to_json.c traverses a directory and spits out a json object.

Files containing valid json are marked as such by wrapping them in
arrays (i.e. arrays are used as literal markers). Other files are
represented as strings.

With the json->tar transformation, GNU tar's --xform acts as a poor
man's jquery.   

json_to_tar < f2.js | gtar tvf --xform s,schema/\(.*\),\1/schema/ --show-transformed-names

You'll need yajl to use this.

Feedback welcome!

gno@8kb.net

bye now.


