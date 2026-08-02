Not() # <command> ...
{
	# Limitation: this cannot be ued recursively.
	Not_has_e=
	case "$-" in *e*) Not_has_e=1 ;; esac

	set +e
	"$@"
	exitstatus=$?
	[ "$Not_has_e" ] && set -e

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
