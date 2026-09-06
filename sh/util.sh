Not() # <command> ...
# Inverses the exit status 0 and 1.  Useful on commands like grep, to make "set -e" work
# when testing that grep should NOT find something.
{
	set +e
	"$@"
	exitstatus=$?
	set -e

	if [ "$exitstatus" = 0 ]
	then
		return 1
	elif [ "$exitstatus" = 1 ]
	then
		return 0
	else
		return "$exitstatus"
	fi
}
