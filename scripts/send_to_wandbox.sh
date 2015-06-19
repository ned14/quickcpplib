#!/bin/bash -x
# Adapted from https://github.com/krzysztof-jusiak/di/blob/cpp14/doc/tools/try_it_online.sh

function json_escape() {
    cat $1 | python -c 'import json,sys,codecs; sys.stdout.write(json.dumps(codecs.getreader("ISO-8859-1")(sys.stdin).read()))'
}

json_wandbox() {
    echo -e -n '{ "code" : '
        json_escape $1

    echo -e -n ',
      "codes": [
          { "file": "'$2'", "code": '
              json_escape $2

    echo -e '}],
      "options": "warning,boost-1.58,c++14",
      "compiler": "clang-head",
      "save": true
    }'
}
json_coliru() {
    echo -e -n '{ "cmd": "", "src": '
             json_escape $2
    echo -e '}'
}

json_wandbox $1 $2 > json_input.json
curl -v -H "Content-type: application/json" -F json_file=@json_input.json http://melpon.org/wandbox/api/compile.json
#URL=`curl -H "Content-type: application/json" -d "\`json $1 $2\`" http://melpon.org/wandbox/api/compile.json | grep url | awk '{print $3}' | tr -d '"'`
#json_coliru $1 $2 > json_input.json
#curl -v --tr-encoding -F json_file=@json_input.json http://coliru.stacked-crooked.com/compile

if [[ $FRAME != "" ]]; then
    echo '<iframe src="'$URL'" frameborder="0" style="height: 100%; width: 100%;" height="100%" width="100%"></iframe>'
else
    echo $URL
fi


