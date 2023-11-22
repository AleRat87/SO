#!/bin/bash

if [ "$#" -ne 1 ]
then
    echo "numar incorect de argumente"
    exit 1
fi
//Se adauga conditiile pentru propozitii cu grep
nr_linii=0

while read linie
do
    nr_linii=$(($nr_linii+1));

done

echo "$nr_linii"
     
