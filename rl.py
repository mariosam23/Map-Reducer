#!/usr/bin/env python3
import imaplib
import smtplib
import email
from email.mime.text import MIMEText

SERVER = "10.196.51.66"
USERNAME = "support"
PASSWORD = "Pierdut$Cont1337"
SUBJECT_FILTER = "Support Ticket"

def process_emails():
    try:
        mail = imaplib.IMAP4_SSL(SERVER)
        mail.login(USERNAME, PASSWORD)

        smtp = smtplib.SMTP(SERVER)

        mail.select("inbox")
        _, message_numbers = mail.search(None, 'ALL')

        if not message_numbers[0]:
            print("No messages found.")
            return

        ticket_number = 1

        for num in message_numbers[0].split():
            _, data = mail.fetch(num, '(RFC822)')
            raw_email = data[0][1]
            email_message = email.message_from_bytes(raw_email)

            from_email = email.utils.parseaddr(email_message['From'])[1]
            subject = email_message['Subject']

            print(f"Processing email from {from_email} with subject: {subject}")

            if SUBJECT_FILTER not in subject:
                print(f"Skipped email: {subject}")
                continue

            sender_domain = from_email.split('@')[1]

            reply_body = (
                f"Hi,\n\nYour ticket named '{subject}' was registered as #{ticket_number}.\n\nThank you for your patience!"
            )
            reply = MIMEText(reply_body)
            reply['From'] = f"{USERNAME}@{sender_domain}"
            reply['To'] = from_email
            reply['Subject'] = f"Re: {subject}"

            print(f"Reply details:\nFrom: {reply['From']}\nTo: {reply['To']}\nSubject: {reply['Subject']}\nBody: {reply_body}")

            try:
                smtp.sendmail(reply['From'], [from_email], reply.as_string())
                print(f"Reply sent to {from_email} for ticket #{ticket_number}")
            except smtplib.SMTPException as e:
                print(f"Failed to send reply to {from_email}: {e}")

            ticket_number += 1

    except _:
        print(f"Error")
    finally:
        if 'smtp' in locals():
            smtp.quit()
        if 'mail' in locals():
            mail.logout()

if __name__ == "__main__":
    process_emails()