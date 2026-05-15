set -e
SCRIPT=$(basename "$0")

Error() # <message>
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
	exit 2
}
