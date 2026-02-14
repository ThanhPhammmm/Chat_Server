Improvement:
- User account deletion
- ACK + Resend
- Resend all message to users who disconnected accidently when logging again

BUG:
- ACK failed
- Inconsistency between server and database
- ACK is sent for all msgs, dont expect that
- There are some places to fix with delimeter problems

Test:
- Check all diagrams
- ACK + Resend
- Resend all message to users who disconnected accidently when logging again
