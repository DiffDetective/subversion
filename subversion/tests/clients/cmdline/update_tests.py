#!/usr/bin/env python
#
#  update_tests.py:  testing update cases.
#
#  Subversion is a tool for revision control. 
#  See http://subversion.tigris.org for more information.
#    
# ====================================================================
# Copyright (c) 2000-2001 CollabNet.  All rights reserved.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution.  The terms
# are also available at http://subversion.tigris.org/license-1.html.
# If newer versions of this license are posted there, you may use a
# newer version instead, at your option.
#
######################################################################

# General modules
import shutil, string, sys, re, os, traceback

# The `svntest' module
try:
  import svntest
except SyntaxError:
  sys.stderr.write('[SKIPPED] ')
  print "<<< Please make sure you have Python 2 or better! >>>"
  traceback.print_exc(None,sys.stdout)
  raise SystemExit

# Quick macro for auto-generating sandbox names
def sandbox(x):
  return "update_tests-" + `test_list.index(x)`

# (abbreviation)
path_index = svntest.actions.path_index
  
 
######################################################################
# Tests
#
#   Each test must return 0 on success or non-zero on failure.


#----------------------------------------------------------------------

# Helper for update_binary_file() test -- a custom singleton handler.
def detect_extra_files(node, extra_files):
  """NODE has been discovered as an extra file on disk.  Verify that
  it matches one of the regular expressions in the EXTRA_FILES list of
  lists, and that its contents matches the second part of the list
  item.  If it matches, remove the match from the list.  If it doesn't
  match, raise an exception."""

  # Baton is of the form:
  #
  #     [ WC_DIR,
  #       [pattern, contents],
  #       [pattern, contents], ... ]

  wc_dir = extra_files.pop(0)

  for pair in extra_files:
    pattern = pair[0]
    contents = pair[1]
    match_obj = re.match(pattern, node.name)
    if match_obj:
      fp = open(os.path.join (wc_dir, node.path))
      real_contents = fp.read()  # suck up contents of a test .png file
      fp.close()
      if real_contents == contents:
        extra_files.pop(extra_files.index(pair)) # delete pattern from list
        return 0

  print "Found unexpected disk object:", node.name
  raise svntest.tree.SVNTreeUnequal



def update_binary_file():
  "update a locally-modified binary file"

  sbox = sandbox(update_binary_file)
  wc_dir = os.path.join (svntest.main.general_wc_dir, sbox)
  
  if svntest.actions.make_repo_and_wc(sbox):
    return 1

  # Add a binary file to the project.
  fp = open("theta.png")
  theta_contents = fp.read()  # suck up contents of a test .png file
  fp.close()

  theta_path = os.path.join(wc_dir, 'A', 'theta')
  fp = open(theta_path, 'w')
  fp.write(theta_contents)    # write png filedata into 'A/theta'
  fp.close()
  
  svntest.main.run_svn(None, 'add', theta_path)  

  # Created expected output tree for 'svn ci'
  output_list = [ [theta_path, None, {}, {'verb' : 'Adding' }] ]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected status tree
  status_list = svntest.actions.get_virginal_status_list(wc_dir, '2')
  for item in status_list:
    item[3]['wc_rev'] = '1'
  status_list.append([theta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '2',
                       'repos_rev' : '2'}])
  expected_status_tree = svntest.tree.build_generic_tree(status_list)

  # Commit the new binary file, creating revision 2.
  if svntest.actions.run_and_verify_commit (wc_dir, expected_output_tree,
                                            expected_status_tree, None,
                                            None, None, None, None, wc_dir):
    return 1

  # Make a backup copy of the working copy.
  wc_backup = wc_dir + 'backup'
  svntest.actions.duplicate_dir(wc_dir, wc_backup)
  theta_backup_path = os.path.join(wc_backup, 'A', 'theta')

  # Make a change to the binary file in the original working copy
  svntest.main.file_append(theta_path, "revision 3 text")
  theta_contents_r3 = theta_contents + "revision 3 text"

  # Created expected output tree for 'svn ci'
  output_list = [ [theta_path, None, {}, {'verb' : 'Sending' }] ]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected status tree
  status_list = svntest.actions.get_virginal_status_list(wc_dir, '3')
  for item in status_list:
    item[3]['wc_rev'] = '1'
  status_list.append([theta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '3',
                       'repos_rev' : '3'}])
  expected_status_tree = svntest.tree.build_generic_tree(status_list)

  # Commit original working copy again, creating revision 3.
  if svntest.actions.run_and_verify_commit (wc_dir, expected_output_tree,
                                            expected_status_tree, None,
                                            None, None, None, None, wc_dir):
    return 1

  # Now start working in the backup working copy:

  # Make a local mod to theta
  svntest.main.file_append(theta_backup_path, "extra theta text")
  theta_contents_local = theta_contents + "extra theta text"

  # Create expected output tree for an update of wc_backup.
  output_list = [ [theta_backup_path, None, {}, {'status' : 'U '}] ]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected disk tree for the update -- 
  #    look!  binary contents, and a binary property!
  my_greek_tree = svntest.main.copy_greek_tree()
  my_greek_tree.append(['A/theta',
                        theta_contents_r3,
                        {'svn:mime-type' : 'application/octet-stream'}, {}])
  expected_disk_tree = svntest.tree.build_generic_tree(my_greek_tree)

  # Create expected status tree for the update.
  status_list = svntest.actions.get_virginal_status_list(wc_backup, '3')
  status_list.append([theta_backup_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '3',
                       'repos_rev' : '3'}])  
  expected_status_tree = svntest.tree.build_generic_tree(status_list)

  # Extra 'singleton' files we expect to exist after the update.
  # In the case, the locally-modified binary file should be backed up
  # to an .orig file.
  #  This is a list of lists, of the form [ WC_DIR,
  #                                         [pattern, contents], ...]
  extra_files = [wc_backup, ['theta.*\.orig', theta_contents_local]]
  
  # Do the update and check the results in three ways.  Pass our
  # custom singleton handler to verify the .orig file; this handler
  # will verify the existence (and contents) of both binary files
  # after the update finishes.
  if svntest.actions.run_and_verify_update(wc_backup,
                                           expected_output_tree,
                                           expected_disk_tree,
                                           expected_status_tree,
                                           detect_extra_files, extra_files,
                                           None, None, 1):
    return 1

  # verify that the extra_files list is now empty.
  if len(extra_files) != 0:
    print "Not all extra reject files have been accounted for:"
    print extra_files
    return 1

  return 0

#----------------------------------------------------------------------

def update_binary_file_2():
  "update to an old revision of a binary files"

  sbox = sandbox(update_binary_file_2)
  wc_dir = os.path.join (svntest.main.general_wc_dir, sbox)
  
  if svntest.actions.make_repo_and_wc(sbox):
    return 1

  # Suck up contents of a test .png file.
  fp = open("theta.png")
  theta_contents = fp.read()  
  fp.close()

  # 102400 is svn_txdelta_window_size.  We're going to make sure we
  # have at least 102401 bytes of data in our second binary file (for
  # no reason other than we have had problems in the past with getting
  # svndiff data out of the repository for files > 102400 bytes).
  # How?  Well, we'll just keep doubling the binary contents of the
  # original theta.png until we're big enough.
  zeta_contents = theta_contents
  while(len(zeta_contents) < 102401):
    zeta_contents = zeta_contents + zeta_contents

  # Write our two files' contents out to disk, in A/theta and A/zeta.
  theta_path = os.path.join(wc_dir, 'A', 'theta')
  fp = open(theta_path, 'w')
  fp.write(theta_contents)    
  fp.close()
  zeta_path = os.path.join(wc_dir, 'A', 'zeta')
  fp = open(zeta_path, 'w')
  fp.write(zeta_contents)
  fp.close()

  # Now, `svn add' those two files.
  svntest.main.run_svn(None, 'add', theta_path, zeta_path)  

  # Created expected output tree for 'svn ci'
  output_list = [ [theta_path, None, {}, {'verb' : 'Adding' }],
                  [zeta_path, None, {}, {'verb' : 'Adding' }] ]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected status tree
  status_list = svntest.actions.get_virginal_status_list(wc_dir, '2')
  for item in status_list:
    item[3]['wc_rev'] = '1'
  status_list.append([theta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '2',
                       'repos_rev' : '2'}])
  status_list.append([zeta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '2',
                       'repos_rev' : '2'}])
  expected_status_tree = svntest.tree.build_generic_tree(status_list)

  # Commit the new binary filea, creating revision 2.
  if svntest.actions.run_and_verify_commit (wc_dir, expected_output_tree,
                                            expected_status_tree, None,
                                            None, None, None, None, wc_dir):
    return 1

  # Make some mods to the binary files.
  svntest.main.file_append (theta_path, "foobar")
  new_theta_contents = theta_contents + "foobar"
  svntest.main.file_append (zeta_path, "foobar")
  new_zeta_contents = zeta_contents + "foobar"
  
  # Created expected output tree for 'svn ci'
  output_list = [ [theta_path, None, {}, {'verb' : 'Sending' }],
                  [zeta_path, None, {}, {'verb' : 'Sending' }] ]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected status tree
  status_list = svntest.actions.get_virginal_status_list(wc_dir, '3')
  for item in status_list:
    item[3]['wc_rev'] = '1'
  status_list.append([theta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '3',
                       'repos_rev' : '3'}])
  status_list.append([zeta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '3',
                       'repos_rev' : '3'}])
  expected_status_tree = svntest.tree.build_generic_tree(status_list)

  # Commit original working copy again, creating revision 3.
  if svntest.actions.run_and_verify_commit (wc_dir, expected_output_tree,
                                            expected_status_tree, None,
                                            None, None, None, None, wc_dir):
    return 1

  # Create expected output tree for an update of wc_backup.
  output_list = [ [theta_path, None, {}, {'status' : 'U '}],
                  [zeta_path, None, {}, {'status' : 'U '}] ]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected disk tree for the update -- 
  #    look!  binary contents, and a binary property!
  my_greek_tree = svntest.main.copy_greek_tree()
  my_greek_tree.append(['A/theta',
                        theta_contents,
                        {'svn:mime-type' : 'application/octet-stream'}, {}])
  my_greek_tree.append(['A/zeta',
                        zeta_contents,
                        {'svn:mime-type' : 'application/octet-stream'}, {}])
  expected_disk_tree = svntest.tree.build_generic_tree(my_greek_tree)

  # Create expected status tree for the update.
  status_list = svntest.actions.get_virginal_status_list(wc_dir, '3')
  for item in status_list:
    item[3]['wc_rev'] = '2'
  status_list.append([theta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '2',
                       'repos_rev' : '3'}])  
  status_list.append([zeta_path, None, {},
                      {'status' : '__',
                       'locked' : ' ',
                       'wc_rev' : '2',
                       'repos_rev' : '3'}])  
  expected_status_tree = svntest.tree.build_generic_tree(status_list)

  # Do an update from revision 2 and make sure that our binary file
  # gets reverted to its original contents.
  return svntest.actions.run_and_verify_update(wc_dir,
                                               expected_output_tree,
                                               expected_disk_tree,
                                               expected_status_tree,
                                               None, None,
                                               None, None, 1,
                                               '-r', '2')


#----------------------------------------------------------------------

def update_missing():
  "update missing items (by name) in working copy"

  sbox = sandbox(update_missing)
  wc_dir = os.path.join (svntest.main.general_wc_dir, sbox)
  
  if svntest.actions.make_repo_and_wc(sbox):
    return 1

  # Remove some files and dirs from the working copy.
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')

  os.remove(mu_path)
  os.remove(rho_path)
  shutil.rmtree(E_path)
  shutil.rmtree(H_path)

  # Create expected output tree for an update of the missing items by name
  output_list = [[mu_path, None, {}, {'status' : 'A '}],
                 [rho_path, None, {}, {'status' : 'A '}],
                 [E_path, None, {}, {'status' : 'A '}],
                 [os.path.join(E_path, 'alpha'), None, {}, {'status' : 'A '}],
                 [os.path.join(E_path, 'beta'), None, {}, {'status' : 'A '}],
                 [H_path, None, {}, {'status' : 'A '}],
                 [os.path.join(H_path, 'chi'), None, {}, {'status' : 'A '}],
                 [os.path.join(H_path, 'omega'), None, {}, {'status' : 'A '}],
                 [os.path.join(H_path, 'psi'), None, {}, {'status' : 'A '}]]
  expected_output_tree = svntest.tree.build_generic_tree(output_list)

  # Create expected disk tree for the update.
  my_greek_tree = svntest.main.copy_greek_tree()
  expected_disk_tree = svntest.tree.build_generic_tree(my_greek_tree)

  # Create expected status tree for the update.
  status_list = svntest.actions.get_virginal_status_list(wc_dir, '1')
  expected_status_tree = svntest.tree.build_generic_tree(status_list)
  
  # Do the update and check the results in three ways.
  return svntest.actions.run_and_verify_update(wc_dir,
                                               expected_output_tree,
                                               expected_disk_tree,
                                               expected_status_tree,
                                               None, None, None, None, 0,
                                               mu_path, rho_path,
                                               E_path, H_path)


########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              update_binary_file,
              update_binary_file_2,
             # update_missing
             ]

if __name__ == '__main__':
  
  ## run the main test routine on them:
  err = svntest.main.run_tests(test_list)

  ## remove all scratchwork: the 'pristine' repository, greek tree, etc.
  ## This ensures that an 'import' will happen the next time we run.
  if os.path.exists(svntest.main.temp_dir):
    shutil.rmtree(svntest.main.temp_dir)

  ## return whatever main() returned to the OS.
  sys.exit(err)


### End of file.
# local variables:
# eval: (load-file "../../../svn-dev.el")
# end:
