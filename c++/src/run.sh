
make

retVal=$?

if [ $retVal -ne 0 ]; then
	echo "Compilation error"
	exit $retVal
fi

./eduRen
