#
# Dotted in by tests/*/EXEC scripts.
#

set -e
SCRIPT=$(basename "$0")
[ "$TEST_VERBOSE" ] && exec 3>&1 || exec 3>/dev/null

Error() # <message>
# Message should start with lowercase letter.
{
	message=$1
	printf >&2 '*** %s: Error: %s\n' "$SCRIPT" "$message"
	[ "$exitstatus" ] && echo >&2 "exitstatus=$exitstatus"
	[ -e list.out ] && {
		printf >&2 '/-------------- stdout:\n'
		cat >&2 list.out
		printf >&2 '\\--------------\n'
	}
	[ -e list.err ] && {
		printf >&2 '/-------------- stderr:\n'
		cat >&2 list.err
		printf >&2 '\\--------------\n'
	}
	exit 1
}

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
