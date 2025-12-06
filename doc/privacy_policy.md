# WinConveyoR Privacy Policy

## Introduction

WinConveyoR is committed to protecting your privacy. This Privacy Policy explains how we collect, use, disclose, and safeguard your information when you use the WinConveyoR package manager software and related services.

This policy applies to:
- WinConveyoR CLI (command-line interface)
- WinConveyoR GUI (graphical user interface) and Community Hub
- Official WinConveyoR package repositories
- WinConveyoR API services

**Effective Date:** December 2025

**Please read this privacy policy carefully.** By using WinConveyoR, you acknowledge that you have read and understood this Privacy Policy and agree to be bound by its terms.


## Information We Collect

### Information You Provide to Us

#### Community Hub Account Information

When you create an account on the WinConveyoR Community Hub, we collect:

- **Email address** - Required for account creation, authentication, and communication
- **Username** - Your chosen display name
- **Password** - Stored in hashed form using secure one-way cryptographic hash functions

#### Voluntary Information

You may choose to provide:
- Profile information (avatar, bio, etc.)
- Package reviews and ratings
- Support requests and feedback

### Information Automatically Collected

#### IP Address Logging

When you connect to the WinConveyoR Community Hub or official repositories, we automatically collect:

- **IP addresses** - Logged for security, abuse prevention, and service optimization
- **Connection timestamps** - Date and time of connections
- **General geographic location** - Derived from IP address (country/region level only)

**Purpose:** IP address logging helps us:
- Detect and prevent abuse, spam, and malicious activity
- Comply with legal requirements
- Monitor system performance and availability
- Debug technical issues

**Retention:** IP address logs are retained for 90 days, after which they are automatically deleted.

#### API Request Logs

Our API servers maintain logs that include:

- Request timestamps
- HTTP methods and endpoints accessed
- Response status codes
- IP addresses
- User agent strings

**Note:** We do NOT track individual user behavior, create user profiles, or use tracking cookies for analytics purposes.

### Information NOT Collected

We explicitly **DO NOT** collect:

- ❌ Telemetry data
- ❌ Automatic crash reports
- ❌ Usage statistics or analytics
- ❌ Package download history (from official repositories)
- ❌ Command-line usage data
- ❌ File system information
- ❌ Personal identification documents


## How We Use Your Information

### Primary Uses

We use the information we collect for the following purposes:

#### Account Management
- Creating and maintaining your Community Hub account
- Authenticating your identity when you log in
- Communicating with you about your account

#### Service Provision
- Providing access to the WinConveyoR Community Hub
- Enabling package downloads and installations
- Processing API requests

#### Security and Fraud Prevention
- Detecting and preventing abuse, spam, and malicious activity
- Identifying and blocking attacks on our infrastructure
- Protecting the integrity of our services

#### Legal Compliance
- Complying with applicable laws and regulations
- Responding to legal requests and preventing harm
- Enforcing our Terms of Service

#### Service Improvement
- Debugging technical issues
- Monitoring system performance
- Optimizing server infrastructure

### We Do NOT Use Your Information For:

- ❌ Advertising or marketing analytics
- ❌ Selling or renting to third parties
- ❌ Creating behavioral profiles
- ❌ Cross-service tracking
- ❌ Personalized advertising


## Data Storage and Security

### Where Your Data is Stored

#### Local Storage

The following data is stored **only on your device**:

- **Package cache** - Downloaded packages stored temporarily
  - Linux: `~/.cache/wcr/` or `/var/cache/wcr/`
  - Windows: `%LOCALAPPDATA%\wcr\cache\`

- **Configuration files** - Your WinConveyoR settings
  - Linux: `/etc/wcr.conf` and `/etc/wcr.d/sources.conf`
  - Windows: `%APPDATA%\wcr\wcr.conf` and `%APPDATA%\wcr\sources.conf`

- **Log files** - Application logs containing warnings and errors
  - Linux: `/var/log/wcr/` or `~/.local/share/wcr/logs/`
  - Windows: `%APPDATA%\wcr\logs\`

**Important:** All local data remains on your device and is never transmitted to our servers unless you explicitly use features requiring server communication (like the Marketplace).

#### Server Storage

Data stored on our servers includes:

- Community Hub account information (email, username, hashed password)
- IP address logs (retained for 90 days)
- API request logs (retained for 90 days)

### Security Measures

We implement industry-standard security measures to protect your information:

#### Technical Safeguards
- **Encryption in transit** - All connections use HTTPS/TLS encryption
- **Secure password hashing** - Passwords are hashed using bcrypt with salt
- **Secure API authentication** - Token-based authentication with rate limiting
- **Regular security updates** - We promptly patch security vulnerabilities

#### Access Controls
- Limited access to production systems
- Authentication required for administrative access
- Audit logging of administrative actions

#### Data Minimization
- We collect only the minimum data necessary
- Automatic deletion of old logs (90-day retention)
- No unnecessary data retention

### Data Breach Notification

In the event of a data breach that affects your personal information, we will:
1. Notify affected users within 72 hours of discovering the breach
2. Provide details about what information was compromised
3. Explain what steps we are taking to address the breach
4. Advise you on actions you can take to protect yourself


## Data Sharing and Disclosure

### We Do Not Sell Your Data

**We will never sell, rent, or trade your personal information to third parties for marketing purposes.**

### Limited Data Sharing

We may share your information only in the following circumstances:

#### Third-Party Service Providers

We use GitHub for:
- Hosting software releases
- Issue tracking and project management
- Source code repository

**GitHub's data practices are governed by their own privacy policy:**  
https://docs.github.com/en/site-policy/privacy-policies/github-privacy-statement

#### Legal Requirements

We may disclose your information if required by law or in response to:
- Valid legal processes (subpoenas, court orders)
- Requests from law enforcement or government agencies
- Circumstances where we believe disclosure is necessary to:
  - Prevent illegal activity or fraud
  - Protect the safety of individuals
  - Protect our legal rights

#### Business Transfers

If WinConveyoR is involved in a merger, acquisition, or sale of assets, your information may be transferred. We will notify you via email and/or prominent notice on our website before your information becomes subject to a different privacy policy.

### No Advertising Networks

We do not integrate with advertising networks or third-party analytics services.


## Self-Hosted Repositories

### Community-Managed Repositories

WinConveyoR allows users and organizations to host their own package repositories using our Docker image. **We have no control over and are not responsible for the privacy practices of self-hosted repositories.**

#### What This Means for You

When you configure WinConveyoR to use a community-managed or self-hosted repository:

- ⚠️ **That repository may collect data** - The repository operator determines their own data collection practices
- ⚠️ **This Privacy Policy does not apply** - Self-hosted repositories are not governed by this policy
- ⚠️ **Review their privacy policy** - Check the repository operator's privacy practices before use

#### Official vs. Community Repositories

| Repository Type | Covered by This Policy | Data Collection |
|-----------------|------------------------|-----------------|
| **Official WinConveyoR repositories** | ✅ Yes | As described in this policy |
| **Self-hosted/community repositories** | ❌ No | Determined by operator |

#### Your Responsibility

When adding a self-hosted repository to your sources configuration, you should:

1. **Review the privacy policy** of the repository operator
2. **Understand what data they collect** (if any)
3. **Assess the trustworthiness** of the repository operator
4. **Use HTTPS connections** when possible for security

#### For Repository Operators

If you operate a self-hosted WinConveyoR repository:

- ✅ **Create your own privacy policy** - Clearly disclose your data practices
- ✅ **Be transparent** - Inform users what data you collect and why
- ✅ **Secure user data** - Implement appropriate security measures
- ✅ **Comply with laws** - Follow applicable privacy regulations (GDPR, CCPA, etc.)

We encourage repository operators to adopt privacy-respecting practices and minimize data collection.


## Your Rights and Choices

### Access and Control Your Data

You have the following rights regarding your personal information:

#### Right to Access
You can request a copy of the personal information we hold about you.

**How to exercise:** Email us via GitHub Issues at https://github.com/StevenGandon/WinConveyoR/issues with the label `privacy`

#### Right to Correction
You can request that we correct inaccurate or incomplete information.

**How to exercise:** Update your profile in the Community Hub settings or contact us via GitHub

#### Right to Deletion
You can request deletion of your personal information, subject to legal retention requirements.

**How to exercise:**
- Delete your Community Hub account in account settings
- Request manual deletion by contacting us via GitHub

**Note:** Some information may be retained in backup systems for up to 90 days after deletion.

#### Right to Data Portability
You can request your data in a machine-readable format.

**How to exercise:** Contact us via GitHub to request a data export

#### Right to Object
You can object to certain processing of your information.

**How to exercise:** Contact us via GitHub with your specific objection

### Managing Your Privacy

#### Local Data Management

You can manage locally stored data at any time:

**Clear cache:**
```bash
# Linux/macOS
rm -rf ~/.cache/wcr/

# Windows
rmdir /s %LOCALAPPDATA%\wcr\cache\
```

**Delete logs:**
```bash
# Linux
rm -rf ~/.local/share/wcr/logs/

# Windows
rmdir /s %APPDATA%\wcr\logs\
```

#### Community Hub Account Deletion

To delete your Community Hub account:

1. Log in to the WinConveyoR Community Hub
2. Go to **Settings** → **Account**
3. Click **Delete Account**
4. Confirm deletion

**Effect:** This will permanently delete your account, email address, and associated data from our servers.

#### Opt-Out of IP Logging

IP address logging is essential for security and cannot be completely disabled. However, you can:

- **Use the CLI only** - CLI operations don't require Community Hub authentication
- **Self-host repositories** - Use local or trusted community repositories
- **Use a VPN** - Route your traffic through a VPN service


## Children's Privacy

WinConveyoR is not intended for use by children under the age of 13 (or the applicable age of digital consent in your jurisdiction).

**We do not knowingly collect personal information from children under 13.**

If you believe we have inadvertently collected information from a child under 13, please contact us immediately via GitHub Issues at https://github.com/StevenGandon/WinConveyoR/issues and we will promptly delete the information upon verification.

**Parents and guardians:** If you become aware that your child has provided us with personal information without your consent, please contact us via GitHub Issues.


## International Data Transfers

### Data Processing Locations

WinConveyoR is developed and operated by a team located in Europe. Your information may be transferred to and processed in countries other than your country of residence.

### Cross-Border Data Transfers

If you are located in the European Economic Area (EEA), United Kingdom, or Switzerland:

- Your data may be transferred to countries outside the EEA
- We ensure appropriate safeguards are in place for such transfers
- Transfers comply with GDPR requirements

### Your Consent

By using WinConveyoR, you consent to the transfer of your information to countries that may have different data protection laws than your country of residence.


## Legal Basis for Processing (GDPR)

If you are located in the European Economic Area (EEA) or United Kingdom, we process your personal data under the following legal bases:

### Consent
- Creating a Community Hub account
- Subscribing to newsletters or notifications

**You may withdraw consent at any time** by deleting your account or contacting us via GitHub.

### Contractual Necessity
- Providing access to the Community Hub
- Delivering packages you've requested
- Authenticating your identity

### Legitimate Interests
- Security and fraud prevention
- Service improvement and optimization
- Legal compliance and protection of rights

### Legal Obligations
- Complying with applicable laws
- Responding to legal requests
- Data retention requirements


## Data Retention

### How Long We Keep Your Data

| Data Type | Retention Period | Reason |
|-----------|------------------|--------|
| **Community Hub account** | Until account deletion | Service provision |
| **IP address logs** | 90 days | Security and debugging |
| **API request logs** | 90 days | Security and debugging |
| **Deleted account data** | 90 days in backups | Backup retention policy |
| **Legal hold data** | As required by law | Legal compliance |

### Automatic Deletion

- IP address logs are **automatically deleted after 90 days**
- API request logs are **automatically deleted after 90 days**
- Cache and temporary files are managed locally by you

### Deletion Requests

When you request deletion of your account:
1. **Immediate:** Account is deactivated and inaccessible
2. **Within 30 days:** Data is permanently deleted from active systems
3. **Within 90 days:** Data is purged from backup systems


## Cookies and Tracking

### Cookie Usage

WinConveyoR uses minimal cookies for essential functionality:

#### Essential Cookies (Required)

| Cookie Name | Purpose | Expiration |
|-------------|---------|------------|
| `session_token` | Authentication session | 24 hours |
| `csrf_token` | Security (CSRF protection) | Session |

**These cookies are necessary for the Marketplace to function and cannot be disabled.**

#### No Tracking Cookies

We **DO NOT** use:
- ❌ Analytics cookies (Google Analytics, etc.)
- ❌ Advertising cookies
- ❌ Social media tracking pixels
- ❌ Third-party tracking scripts

### Browser Storage

The WinConveyoR GUI may store preferences in your browser's local storage:
- Theme preferences (dark/light mode)
- Language settings
- UI preferences

**This data stays in your browser and is never transmitted to our servers.**

### Do Not Track

We respect "Do Not Track" (DNT) browser signals, though we don't engage in tracking regardless of DNT settings.


## Changes to This Privacy Policy

### Notification of Changes

We may update this Privacy Policy from time to time to reflect:
- Changes to our data practices
- Legal or regulatory requirements
- New features or services

### How We Notify You

When we make material changes to this Privacy Policy, we will:

1. **Update the "Last Updated" date** at the top of this document
2. **Notify Community Hub users via email** if you have an account
3. **Post a prominent notice** on GitHub and in the application
4. **Provide at least 30 days notice** before changes take effect

### Your Continued Use

Your continued use of WinConveyoR after changes take effect constitutes acceptance of the updated Privacy Policy.

### Review Previous Versions

Previous versions of this Privacy Policy are available in our GitHub repository:  
https://github.com/StevenGandon/WinConveyoR/blob/main/PRIVACY_POLICY.md


## Contact Us

### Privacy Questions or Concerns

If you have questions, concerns, or requests regarding this Privacy Policy or our data practices, please contact us via GitHub:

#### GitHub Issues (Primary Contact Method)
For privacy-related inquiries, bug reports, or general questions:

**https://github.com/StevenGandon/WinConveyoR/issues**

When opening a privacy-related issue:
1. Create a new issue
2. Use the label `privacy` or `question`
3. Clearly describe your inquiry or concern
4. Include relevant details (but do NOT include personal information in public issues)

**For sensitive privacy matters involving your personal data:**
- Open an issue and request private communication
- We will reach out to you privately to handle your request

#### Discord Community
For general community discussions:

[Join WinConveyoR Discord](#)

#### Repository
Main project repository:

**https://github.com/StevenGandon/WinConveyoR**

### Response Time

We aim to respond to privacy inquiries within:
- **48 hours** - Initial acknowledgment on GitHub
- **30 days** - Complete response to data requests

### Data Protection Officer

For GDPR-related inquiries, contact us via GitHub Issues at:

**https://github.com/StevenGandon/WinConveyoR/issues**

Use the label `privacy` or `gdpr` for these requests.


## Specific Regional Rights

### European Union (GDPR)

If you are located in the EU, you have additional rights under the General Data Protection Regulation (GDPR):

- **Right to lodge a complaint** with your local data protection authority
- **Right to object to processing** based on legitimate interests
- **Right to restriction of processing** in certain circumstances
- **Right to data portability** in machine-readable format

### California (CCPA)

If you are a California resident, you have rights under the California Consumer Privacy Act (CCPA):

- **Right to know** what personal information we collect and how it's used
- **Right to delete** your personal information (with exceptions)
- **Right to opt-out** of sale of personal information (**we do not sell your information**)
- **Right to non-discrimination** for exercising your privacy rights

**To exercise your CCPA rights:** Contact us via GitHub Issues at https://github.com/StevenGandon/WinConveyoR/issues

### United Kingdom (UK GDPR)

UK residents have rights under UK GDPR similar to EU GDPR rights listed above.

### Other Jurisdictions

We comply with applicable privacy laws in all jurisdictions where WinConveyoR is used. If your region has specific privacy rights not listed here, please contact us via GitHub Issues.


## Open Source Transparency

### Our Commitment

As an open-source project, WinConveyoR is committed to transparency:

- ✅ **Source code is public** - Review our code on GitHub
- ✅ **Privacy policy is versioned** - Track changes in our repository
- ✅ **Community input welcome** - Submit issues or pull requests
- ✅ **No hidden tracking** - What you see in the code is what we do

### Audit Our Practices

You can verify our privacy practices by:

1. **Reviewing the source code:**  
   https://github.com/StevenGandon/WinConveyoR

2. **Examining network requests** made by the application

3. **Inspecting local data storage** on your device

4. **Checking our server code** (API implementation is open source)

### Community Accountability

We encourage security researchers and privacy advocates to:
- Audit our code for privacy issues
- Report concerns through responsible disclosure
- Propose improvements via pull requests


## Appendix: Definitions

**Personal Information / Personal Data**
: Information that identifies, relates to, describes, or could reasonably be linked to you.

**Processing**
: Any operation performed on personal data, including collection, storage, use, or deletion.

**Controller**
: The entity that determines the purposes and means of processing personal data (WinConveyoR Team for official services).

**Processor**
: An entity that processes personal data on behalf of the controller.

**Data Subject**
: An individual whose personal data is being processed (you, the user).

**Community Hub**
: The WinConveyoR graphical user interface (GUI) for browsing and installing packages.

**Self-Hosted Repository**
: A package repository operated by a third party using WinConveyoR's Docker image.

**Official Repository**
: A package repository operated by the WinConveyoR Team.

---

## Document Information

**Privacy Policy Version:** 1.0  
**Effective Date:** December 2025  
**Last Updated:** December 2025

**License:** This Privacy Policy is part of the WinConveyoR project documentation.

**GitHub Repository:**  
https://github.com/StevenGandon/WinConveyoR

**Questions?** Contact us via GitHub Issues:  
https://github.com/StevenGandon/WinConveyoR/issues

---

**© 2025 WinConveyoR Team. All rights reserved.**