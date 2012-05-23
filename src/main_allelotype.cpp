//============================================================================
// Author      : Melissa Gymrek
//============================================================================
#include <err.h>
#include <error.h>
#include <getopt.h>
#include <iostream>
#include <RInside.h>
#include <stdlib.h>
#include <string>

#include "common.h"
#include "Genotyper.h"
#include "NoiseModel.h"
#include "ReadContainer.h"
#include "runtime_parameters.h"

using namespace std;

void show_help(){
  const char* help = "\n\nTo train the genotyping noise model from a set of aligned reads:\n" \
"allelotype --command train [OPTIONS] --bam <input.bam> --noise_model <noisemodel.txt>\n\n" \
"To run str profiling on a set of aligned reads:\n" \
"allelotype --command classify [OPTIONS] --bam <input.bam> --noise_model <noisemodel.txt> [--no-rmdup] --out <output_prefix> --sex [M|F|U]\n\n" \
"To run training and classification on a set of aligned reads:\n" \
"allelotype --command both [OPTIONS] --bam <input.bam> --noise_model <noisemodel.txt> [--no-rmdup] --out <output_prefix> --sex [M|F|U]\n\n" \
"To allelotype without using a stutter noise model:\n" \
"allelotype simple [OPTIONS] --bam <input.bam> [--no-rmdup] --out <output_prefix> --sex [M|F|U]\n\n" \
"Options:\n" \
"--no-rmdup: don't remove pcr duplicates before allelotyping\n" \
"--min-het-freq: minimum frequency to make a heterozygous call\n" \
"--sex: Gender of sample, M=male, F=female, U=unknown\n" \
"-h: display this message\n" \
"-v: print out helpful progress messages\n\n";
  cout << help;
  exit(1);
}

/*
 * parse the command line options
 */
void parse_commandline_options(int argc,char* argv[]) {
	enum LONG_OPTIONS{
	  OPT_COMMAND,
	  OPT_BAM,
	  OPT_OUTPUT,
	  OPT_NORMDUP,
	  OPT_SEX,
	  OPT_NOISEMODEL,
	  OPT_UNIT,
	  OPT_HELP,
	  OPT_VERBOSE,
	  OPT_DEBUG,
	  OPT_NO_INCLUDE_FLANK,
	  OPT_PROFILE,
	  OPT_PRINT_READS,
	  OPT_MIN_HET_FREQ,
	};

	int ch;
	int option_index = 0;

	static struct option long_options[] = {
	  {"reads", 0, 0, OPT_PRINT_READS},
	  {"bam", 1, 0, OPT_BAM},
	  {"command", 1, 0, OPT_COMMAND},
	  {"out", 1, 0, OPT_OUTPUT},
	  {"no-rmdup", 0, 0, OPT_NORMDUP},
	  {"sex", 1, 0, OPT_SEX},
	  {"noise_model", 1, 0, OPT_NOISEMODEL},
	  {"unit", 0, 0, OPT_UNIT},
	  {"help", 1, 0, OPT_HELP},
	  {"debug", 0, 0, OPT_DEBUG},
	  {"no-include-flank",0,0,OPT_NO_INCLUDE_FLANK},
	  {"profile",0,0,OPT_PROFILE},
	  {"min-het-freq",1,0,OPT_MIN_HET_FREQ},
	  {NULL, no_argument, NULL, 0},
	};

	ch = getopt_long(argc,argv,"hv?",
			 long_options,&option_index);
	while (ch != -1) { 
	  switch(ch){
	  case OPT_PRINT_READS:
	    print_reads++;
	    user_defined_arguments_allelotyper += "print-reads=True;";
	    break;
	  case OPT_BAM:
	    bam_file = string(optarg);
	    user_defined_arguments_allelotyper += "bam=";
	    user_defined_arguments_allelotyper += bam_file;
	    user_defined_arguments_allelotyper += ";";
	    break;
	  case OPT_NO_INCLUDE_FLANK:
	    include_flank=false;
	    user_defined_arguments_allelotyper += "no-include-flank=True;";
	    break;
	  case OPT_PROFILE:
	    profile++;
	    break;
	  case OPT_MIN_HET_FREQ:
	    min_het_freq = atof(optarg);
	    user_defined_arguments_allelotyper += "min-het-freq=";
	    user_defined_arguments_allelotyper += string(optarg);
	    user_defined_arguments_allelotyper += ";";
	    break;
	  case OPT_COMMAND:
	    command = string(optarg);
	    user_defined_arguments_allelotyper += "command=";
	    user_defined_arguments_allelotyper += command;
	    user_defined_arguments_allelotyper += ";";
	    if ((command != "train") & (command != "classify") &
		(command != "both") & (command != "simple")) {
	      cerr << "\n\nERROR: Command " << command << " is invalid. Command must be one of: train, classify, both, simple";
	      show_help();
	      exit(1);
	    }
	    break;
	  case OPT_OUTPUT:
	    output_prefix = string(optarg);
	    user_defined_arguments_allelotyper += "out=";
	    user_defined_arguments_allelotyper += output_prefix;
	    user_defined_arguments_allelotyper += ";";
	    break;
	  case OPT_NORMDUP:
	    rmdup = false;
	    user_defined_arguments_allelotyper += "rmdup=False;";
	    break;
	  case OPT_SEX:
	    if (string(optarg) != "F" && string(optarg) != "M" && string(optarg) != "U") {
	      errx(1, "--sex must be F,M, or U");
	    }
	    if (string(optarg) == "F") male = false;
	    if (string(optarg) == "U") sex_unknown = true;
	    
	    user_defined_arguments_allelotyper += "sex=";
	    user_defined_arguments_allelotyper += string(optarg);
	    user_defined_arguments_allelotyper += ";";
	    sex_set++;
	    break;
	  case OPT_NOISEMODEL:
	    // TODO refuse to over write file if command is "train" or "both"
	    noise_model = string(optarg);
	    if ((command == "train" || command == "both")&&fexists(noise_model.c_str())) {
	      errx(1,"Cannot write to specified noise model file. This file already exists.");
	    }
	    user_defined_arguments_allelotyper += "noise-model=";
	    user_defined_arguments_allelotyper += string(optarg);
	    user_defined_arguments_allelotyper += ";";
	    break;
	  case OPT_UNIT:
	    unit = true;
	    user_defined_arguments_allelotyper += "unit=True;";
	    break;
	  case 'v':
	  case OPT_VERBOSE:
	    my_verbose++;
	    break;
	  case 'h':
	  case OPT_HELP:
	    show_help();
	    exit(1);
	    break;
	  case OPT_DEBUG:
	    debug = true;
	    break;
	  case '?':
	    show_help();
	    exit(1);
	    break;
	  default:
	    show_help();
	    exit(1);
	  }
	  ch = getopt_long(argc,argv,"hv?",
			   long_options,&option_index);

	} 

	// any arguments left over are extra
	if (optind < argc) {
	  cerr << "\n\nERROR: Unnecessary leftover arguments";
	  show_help();
	  exit(1);
	}
	// check that we have the mandatory parameters
	if (command.empty()) {
	  cerr << "\n\nERROR: Must specify a command";
	  show_help();
	  exit(1);
	}
	if (command == "train") {
	  if (bam_file.empty() || noise_model.empty()) {
	    cerr << "\n\nERROR: Required arguments are missing. Please specify a bam file and an output prefix";
	    show_help();
	    exit(1);
	  }
	  male = true;
	}
	if (command == "classify") {
	  if (bam_file.empty() || noise_model.empty()
	      or output_prefix.empty() || !sex_set) {
	    cerr << "\n\nERROR: Required arguments are missing. Please specify a bam file, output prefix, noise model, and gender";
	    show_help();
	    exit(1);
	  }
	}
	if (command == "both" || command == "simple") {
	  if (bam_file.empty() || output_prefix.empty() || !sex_set)  {
	    cerr << "\n\nERROR: Required arguments are missing. Please specify a bam file, output prefix, and gender";
	    show_help();
	    exit(1);
	  }
	}
	// check that parameters make sense
	if ((command == "train" || command == "both") && !male) {
	  cerr << "\n\nERROR: Cannot train on female sample";
	  show_help();
	  exit(1);
	}
}

/* copied from common.h */
bool fexists(const char *filename) {
  ifstream ifile(filename);
  return ifile;
}

int main(int argc,char* argv[]) {
  /* parse command line options */
  parse_commandline_options(argc,argv);

  if (my_verbose) cout << "Running allelotyping with command "
		    << command << endl;

  /* initialize noise model */
  RInside R(argc, argv);
  NoiseModel nm(&R);

  /* Add reads to read container */
  if (my_verbose) cout << "Adding reads to read container" << endl;
  ReadContainer read_container;
  read_container.AddReadsFromFile(bam_file);

  /* Perform pcr dup removal if specified */
  if (rmdup) {
    if (my_verbose) cout << "Performing pcr duplicate removal" << endl;
    read_container.RemovePCRDuplicates();
  }

  /* Train/classify */
  if (command == "train" or command == "both") {
    if (my_verbose) cout << "Training noise model..." << endl;
    nm.Train(&read_container);
    nm.WriteNoiseModelToFile(noise_model);
  } else if (command == "classify") {
    if (!nm.ReadNoiseModelFromFile(noise_model)) {
      errx(1,"Error reading noise file");
    }
  }

  if (command == "classify" or command == "both") {
    if (my_verbose) cout << "Classifying allelotypes..." << endl;
    Genotyper genotyper(&nm, male, false);
    genotyper.Genotype(read_container,
		       output_prefix + ".genotypes.tab");
  }
  if (command == "simple") {
    if (my_verbose) cout << "Classifying allelotypes..." << endl;
    Genotyper genotyper(&nm, male, true);
    genotyper.Genotype(read_container,
		       output_prefix + ".genotypes.tab");
  }
  return 0;
}
