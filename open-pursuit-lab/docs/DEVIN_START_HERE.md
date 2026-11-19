# Devin handoff

Use this as the first task message after uploading the project or publishing a
real repository. No remote repository URL or credentials are supplied here.

> Work on Open Pursuit Lab, an early UE5 C++ street racing/pursuit project.
> Read README.md, docs/STATUS.md, docs/ARCHITECTURE.md and docs/BACKLOG.md first.
> Run `python3 tools/verify.py --report evidence/devin-baseline.json` and report
> the actual result. The current real prototype is portable policy code; the UE5
> project is unverified scaffold. Do not describe it as a playable game.
>
> Start with UE-01 only. Check for a licensed UE 5.6 installation/source build
> and supported compiler before editing integration code. Record the exact build
> ID. If no engine is available, report the missing prerequisite and complete only
> independently verifiable source review; do not invent build logs or task closure.
>
> Make one focused PR containing the changes and the UE-01 acceptance artifacts.
> Keep the shared pursuit logic in the portable core. Do not add map generation,
> 100 physical cars, weather, persistence or unrequested infrastructure.
> Run the portable tests after changes. Separate exact policy results from UE
> physics results and label each unexecuted check. Update docs/STATUS.md only from
> observed evidence. Include command lines, exit codes and remaining blockers.

## Review checklist

- Are the task dependencies actually satisfied?
- Can another developer reproduce the reported command and see the same artifact?
- Are engine version, map revision, seed and environment recorded?
- Are failures and unexecuted checks visible?
- Is every public claim supported by a file, build or test report?
- Are assets licensed and integration changes within the task scope?
