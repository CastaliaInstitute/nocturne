# Mini App: Seed Bridge

Small deterministic seed bridge demo that shows how a user session and seed state can
be passed through scheduling stages.

- Inputs: synthetic seed + mode state
- Outputs: monotonic plan ids and stable replay output
- Use: integration smoke test for deterministic behavior

The implementation is intended as a reference blueprint for SDK users adding their
own runtime front-ends.
