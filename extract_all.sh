#!/bin/sh

for d in models/*/*; do
	echo "Extracting $d"
	sh extract_model_code.sh $d
done
