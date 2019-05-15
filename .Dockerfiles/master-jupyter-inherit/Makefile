SHELL := /usr/local/bin/bash -o pipefail
NAME = sourceryinstitute/opencoarrays_jupyter
IMAGE_NAME=$(NAME)
RUN_TESTS=true

#UBADGER_ENDPOINT = "https://hooks.microbadger.com/images/sourceryinstitute/jupyter-caf-kernel/CEL6DIVw-OVbUIXieZhZwvDXl3I="
GIT_MASTER_HEAD_SHA:=$(shell git rev-parse --short=12 --verify HEAD)

.PHONY: help build push run runclean clean
.DEFAULT_GOAL := help

# Add the following 'help' target to your Makefile
# And add help text after each target name starting with '\#\#'

help: ## This help message
	@echo -e "$$(grep -hE '^\S+:.*##' $(MAKEFILE_LIST) | sed -e 's/:.*##\s*/:/' -e 's/^\(.\+\):\(.*\)/\\x1b[36m\1\\x1b[m:\2/' | column -c2 -t -s :)"

build: $(subst /,_,$(NAME))-build.stamp ## Build docker file. Depending on which packages are rebuilt this may take a long time

$(subst /,_,$(NAME))-build.stamp: Dockerfile .dockerignore hooks/build ## Target for doing & timestamping the build
	touch $@
	IMAGE_NAME=$(IMAGE_NAME) hooks/build 2>&1 | tee $(subst stamp,log,$@) || rm -rf $@
	docker tag $(NAME) $(NAME):$(GIT_MASTER_HEAD_SHA)
	docker tag $(NAME) $(NAME):latest

push: $(subst /,_,$(NAME))-push.stamp ## Push prebuild docker image up to hub.docker.com

$(subst /,_,$(NAME))-push.stamp: $(subst /,_,$(NAME))-build.stamp Dockerfile .dockerignore hooks/build ## Target for doing & timestamping the push
	touch $@
	docker push $(NAME):latest | tee $(subst stamp,log,$@) || rm -rf $@
	docker push $(NAME):$(GIT_MASTER_HEAD_SHA) | tee -a $(subst stamp,log,$@) || rm -rf $@
	-curl -X POST "$(UBADGER_ENDPOINT)"

run: ## Target to run jupyter-hub docker server
	docker run -i -t -p 8888:8888 $(NAME) | tee $(subst /,_,$(NAME))-run.log

runclean: ## Target to run jupyter-hub docker server, discarding changes
	docker run --rm -i -t -p 8888:8888 $(NAME) | tee $(subst /,_,$(NAME))-runclean.log

clean:
	-rm -rf $(subst /,_,$(NAME))*.stamp
	-find . -name 'Untitled*.ipynd' -exec rm -f {} \;
