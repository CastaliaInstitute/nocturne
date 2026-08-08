# Branch Protection Notes

Current repository branch baseline:
- Protect `main` in GitHub repository settings
- Require at least one approving review
- Require linear history for maintainability
- Require at least one successful status check (`CI`)
- Require signed commits if policy requires

Create or review branch protection in:

- Settings → Branches → Branch protection rules
- Pattern: `main`

Recommended required checks:

- `CI`

Fallback operational behavior:
- No direct pushes should occur to protected branches.
- Temporary exceptions require maintainer approval.
