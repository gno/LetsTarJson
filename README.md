JSON tar. 

Been getting intimate with daemontools as of late. The part of djb's
mental model that's left the greatest impression on me is the
treatment of directories as key value stores. 

json_to_tar.c reads an json value (which must be an object) and
serializes it in tar format. 

to_json.c iterates a directory hierarchy and spits out a json object.

Files containing valid json are wrapped in 1 element arrays, to avoid
heavyweight escaping.

With the json->tar transformation, GNU's --xform acts as a poor man's jquery. 

json_to_tar < f2.js | gtar tvf --xform s,schema/

You'll need yajl to use this.

Feedback welcome!


