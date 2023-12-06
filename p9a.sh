#!/bin/bash

if [ "$#" -ne 1 ]
    then echo "Numar insuficient de argumente"
    exit 1
fi

nr_linii=0
character=$1

while IFS= read -r linie
do
    expr="^[A-Z][A-Za-z0-9[:space:],.!?]*[?!.]$"
    if echo "$linie" | grep -q "$character";
    then 
        if echo "$linie" | grep -qE "$expr" && ! echo "$linie" | grep -q ", si";
            then nr_linii=$(($nr_linii+1));
        fi
    fi
done

echo "$nr_linii"
     
