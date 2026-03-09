def run(ctx, args):
  # Disable the creation of perf data files (e.g., in /tmp/hsperfdata_...).
  # When tests are run in parallel, these files can cause "locked by another process"
  # warnings. This pollutes stdout/stderr and causes the test's output diff to fail.
  if args.jvm:
    args.runtime_option.append("-XX:-UsePerfData")
  ctx.default_run(args)
